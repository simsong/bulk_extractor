#include "config.h"

// if liblightgrep isn't present, compiles to nothing
#ifdef HAVE_LIBLIGHTGREP

#include "be13_api/bulk_extractor_i.h"
#include "beregex.h"
#include "histogram.h"
#include "pattern_scanner.h"

#include <lightgrep/api.h>

#include <iostream>
#include <algorithm>
#include <limits>
#include <fstream>

#include <iostream>

#ifdef LGBENCHMARK
#include <chrono>
#endif

namespace {
  const char* DefaultEncodingsCStrings[] = {"UTF-8", "UTF-16LE"};
  const unsigned int NumDefaultEncodings = 2;
}

bool PatternScanner::handleParseError(const Handler& h, LG_Error* err) const {
  cerr << "Parse error on '" << h.RE << "' in " << Name
       << ": " << err->Message << endl;
  return false;
}

void PatternScanner::shutdown(const scanner_params&) {
  for (vector<const Handler*>::iterator itr(Handlers.begin()); itr != Handlers.end(); ++itr) {
    delete *itr;
  }
}
/*********************************************************/

LightgrepController::LightgrepController()
: ParsedPattern(lg_create_pattern()),       // Reuse the parsed pattern data structure for efficiency
  Fsm(lg_create_fsm(1 << 20)),              // Reserve space for 1M states in the automaton--will grow if needed
  PatternInfo(lg_create_pattern_map(1000)), // Reserve space for 1000 patterns in the pattern map
  Prog(0),
  Scanners()
{
}

LightgrepController::~LightgrepController() {
  lg_destroy_pattern(ParsedPattern);
  lg_destroy_pattern_map(PatternInfo);
  lg_destroy_program(Prog);
}

LightgrepController& LightgrepController::Get() {
  // Meyers Singleton. c.f. Effective C++ by Scott Meyers
  static LightgrepController controller;
  return controller;
}

bool LightgrepController::addScanner(PatternScanner& scanner) {
  // Add patterns and handlers from a Scanner to the centralized automaton
  LG_Error* lgErr = 0;

  unsigned int patBegin = numeric_limits<unsigned int>::max(),
               patEnd = 0;

  int idx = -1;

  // iterate all the scanner's handlers
  for (vector<const Handler*>::const_iterator h(scanner.handlers().begin()); h != scanner.handlers().end(); ++h) {
    bool good = false;
    if (lg_parse_pattern(ParsedPattern, (*h)->RE.c_str(), &(*h)->Options, &lgErr)) { // parse the pattern
      for (vector<string>::const_iterator enc((*h)->Encodings.begin()); enc != (*h)->Encodings.end(); ++enc) {
        idx = lg_add_pattern(Fsm, PatternInfo, ParsedPattern, enc->c_str(), &lgErr); // add the pattern for each given encoding
        if (idx >= 0) {
          // add the handler callback to the pattern map, associated with the pattern index
          lg_pattern_info(PatternInfo, idx)->UserData = const_cast<void*>(static_cast<const void*>(&((*h)->Callback)));
          patBegin = std::min(patBegin, static_cast<unsigned int>(idx));
          good = true;
        }
      }

//      std::cerr << '\t' << (int)((*h)->Options.FixedString) << '\t' << (int)((*h)->Options.CaseInsensitive) << std::endl;
    }
    if (!good) {
      if (scanner.handleParseError(**h, lgErr)) {
        lg_free_error(lgErr);
        lgErr = 0;
      }
      else {
        return false;
      }      
    }
  }
  patEnd = lg_pattern_map_size(PatternInfo);
  // record the range of this scanner's patterns in the central pattern map
  scanner.patternRange() = make_pair(patBegin, patEnd);
  Scanners.push_back(&scanner);
  return true;
}

bool LightgrepController::addUserPatterns(PatternScanner& scanner, CallbackFnType* callbackPtr, const FindOpts& user) {
  // Add patterns specified as keywords by the user
  // Similar to above, but does not have a handler per pattern
  unsigned int patBegin = lg_pattern_map_size(PatternInfo),
               patEnd = 0;

  LG_KeyOptions opts;
  opts.FixedString = 0;
  opts.CaseInsensitive = 0;

  LG_Error *err = 0;

  // Add patterns from files
  for (vector<string>::const_iterator itr(user.Files.begin()); itr != user.Files.end(); ++itr) {
    ifstream file(itr->c_str(), ios::in);
    if (!file.is_open()) {
      cerr << "Could not open pattern file '" << *itr << "'." << endl;
      return false;
    }
    string contents = string(istreambuf_iterator<char>(file), istreambuf_iterator<char>());

    const char* contentsCStr = contents.c_str();
    // Add all the patterns from the files in one fell swoop
    if (lg_add_pattern_list(Fsm, PatternInfo, contentsCStr, itr->c_str(), DefaultEncodingsCStrings, 2, &opts, &err) < 0) {
      vector<string> lines;
      istringstream input(contents);
      string line;
      while (input) {
        getline(input, line);
        lines.push_back(line);
      }
      LG_Error* cur(err);
      while (cur) {
        cerr << "Error in " << *itr << ", line " << cur->Index+1 << ", pattern '" << lines[cur->Index]
          << "': " << cur->Message << endl;
        cur = cur->Next;
      }
      lg_free_error(err);
      return false;
    }
  }
  // add patterns from single command-line arguments
  for (vector<string>::const_iterator itr(user.Patterns.begin()); itr != user.Patterns.end(); ++itr) {
    bool good = false;
    if (lg_parse_pattern(ParsedPattern, itr->c_str(), &opts, &err)) {
      for (unsigned int i = 0; i < NumDefaultEncodings; ++i) {
        if (lg_add_pattern(Fsm, PatternInfo, ParsedPattern, DefaultEncodingsCStrings[i], &err) >= 0) {
          good = true;
        }
      }
    }
    if (!good) {
      cerr << "Error on '" << *itr << "': " << err->Message << endl;
      lg_free_error(err);
      return false;
    }
  }
  patEnd = lg_pattern_map_size(PatternInfo);
  for (unsigned int i = patBegin; i < patEnd; ++i) {
    lg_pattern_info(PatternInfo, i)->UserData = const_cast<void*>(static_cast<const void*>(callbackPtr));
  }
  scanner.patternRange() = make_pair(patBegin, patEnd);
  Scanners.push_back(&scanner);
  return true;
}

void LightgrepController::regcomp() {
  LG_ProgramOptions progOpts;
  progOpts.Determinize = 1;
  // Create an optimized, immutable form of the accumulated automaton
  Prog = lg_create_program(Fsm, &progOpts);
  lg_destroy_fsm(Fsm);

  cerr << lg_pattern_map_size(PatternInfo) << " lightgrep patterns, logic size is " << lg_program_size(Prog) << " bytes, " << Scanners.size() << " active scanners" << std::endl;
  #ifdef LGBENCHMARK
  cerr << "timer second ratio " << chrono::high_resolution_clock::period::num << "/" <<
    chrono::high_resolution_clock::period::den << endl;
  #endif
}

struct HitData {
  // Everything we need for processing a hit
  LightgrepController* lgc;
  const vector<PatternScanner*>* scannerTable;
  const scanner_params* sp;
  const recursion_control_block* rcb;
};

void gotHit(void* userData, const LG_SearchHit* hit) {
  #ifdef LGBENCHMARK
  // no callback, just increment hit counter
  ++(*static_cast<uint64_t*>(userData));
  #else
  // trampoline back into LightgrepController::processHit() from the void* userData
  HitData* hd(static_cast<HitData*>(userData));
  hd->lgc->processHit(*hd->scannerTable, *hit, *hd->sp, *hd->rcb);
  #endif
}

void LightgrepController::scan(const scanner_params& sp, const recursion_control_block &rcb) {
  // Scan the sbuf for pattern hits, invoking various scanners' handlers as hits are encountered
  if (!Prog) {
    // we had no valid patterns, do nothing
    return;
  }
  // First, clone all the scanners so that there's no shared data between threads
  vector<PatternScanner*> scannerTable(lg_pattern_map_size(PatternInfo)); // [Keyword Index -> scanner], no ownership
  vector<PatternScanner*> scannerList;                                    // ownership list
  for (vector<PatternScanner*>::const_iterator itr(Scanners.begin()); itr != Scanners.end(); ++itr) {
    PatternScanner *s = (*itr)->clone();
    scannerList.push_back(s);
    for (unsigned int i = s->patternRange().first; i < s->patternRange().second; ++i) {
      scannerTable[i] = s;
    }
    s->initScan(sp); // let the scanner know we're about to scan an sbuf
  }
  LG_ContextOptions ctxOpts;
  ctxOpts.TraceBegin = 0xffffffffffffffff;
  ctxOpts.TraceEnd   = 0;

  LG_HCONTEXT ctx = lg_create_context(Prog, &ctxOpts); // create a search context; cannot be shared, so local to scan

  const sbuf_t &sbuf = sp.sbuf;

  HitData callbackInfo = { this, &scannerTable, &sp, &rcb };
  void*   userData = &callbackInfo;

  #ifdef LGBENCHMARK // perform timings of lightgrep search functions only -- no callbacks
  uint64_t hitCount = 0;
  userData = &hitCount; // switch things out for a counter

  auto startClock = std::chrono::high_resolution_clock::now();
  // std::cout << "Starting block " << sbuf.pos0.str() << std::endl;
  #endif

  // search the sbuf in one go
  // the gotHit() function will be invoked for each pattern hit
  if (lg_search(ctx, (const char*)sbuf.buf, (const char*)sbuf.buf + sbuf.pagesize, 0, userData, gotHit) < numeric_limits<uint64_t>::max()) {
    // resolve potential hits that want data into the sbuf margin, without beginning any new hits
    lg_search_resolve(ctx, (const char*)sbuf.buf + sbuf.pagesize, (const char*)sbuf.buf + sbuf.bufsize, sbuf.pagesize, userData, gotHit);
  }
  // flush any remaining hits; there's no more data
  lg_closeout_search(ctx, userData, gotHit);

  #ifdef LGBENCHMARK
  auto endClock = std::chrono::high_resolution_clock::now();
  auto t = endClock - startClock;
  double seconds = double(t.count() * chrono::high_resolution_clock::period::num) / chrono::high_resolution_clock::period::den;
  double bw = double(sbuf.pagesize) / (seconds * 1024 * 1024);
  std::stringstream buf;
  buf << " ** Time: " << sbuf.pos0.str() << '\t' << sbuf.pagesize << '\t' << t.count() << '\t' << seconds<< '\t' << hitCount << '\t' << bw << std::endl;
  std::cout << buf.str();
//  std::cout.flush();
  #endif

  lg_destroy_context(ctx);

  // don't call PatternScanner::shutdown() on these! that only happens on prototypes
  for (vector<PatternScanner*>::const_iterator itr(scannerList.begin()); itr != scannerList.end(); ++itr) {
    (*itr)->finishScan(sp); // let the scanner know we're done with the sbuf
    delete *itr;
  }
}

void LightgrepController::processHit(const vector<PatternScanner*>& sTbl, const LG_SearchHit& hit, const scanner_params& sp, const recursion_control_block& rcb) {
  // lookup the handler's callback functor in the pattern map, then invoke it
  CallbackFnType* cbPtr(static_cast<CallbackFnType*>(lg_pattern_info(PatternInfo, hit.KeywordIndex)->UserData));
  ((*sTbl[hit.KeywordIndex]).*(*cbPtr))(hit, sp, rcb); // ...yep...
}

unsigned int LightgrepController::numPatterns() const {
  return lg_pattern_map_size(PatternInfo);
}

/*********************************************************/

void scan_lg(PatternScanner& scanner, const class scanner_params &sp, const recursion_control_block &rcb) {
  // utility implementation of the normal scan function for a PatternScanner instance
  switch (sp.phase) {
  case scanner_params::PHASE_STARTUP:
    scanner.startup(sp);
    break;
  case scanner_params::PHASE_INIT:
    scanner.init(sp);
    if (!LightgrepController::Get().addScanner(scanner)) {
      // It's fine for user patterns not to parse, but there's no excuse for a scanner so exit.
      cerr << "Aborting. Fix pattern or disable scanner to continue." << endl;
      exit(EXIT_FAILURE);
    }
    break;
  case scanner_params::PHASE_SHUTDOWN:
    scanner.shutdown(sp);
    break;
  default:
    break;
  }
}

/*********************************************************/

#endif // HAVE_LIBLIGHTGREP
