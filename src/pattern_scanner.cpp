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
: ParsedPattern(lg_create_pattern()), Fsm(lg_create_fsm(1 << 20)), PatternInfo(lg_create_pattern_map(1000)), Prog(0), Scanners()
{
}

LightgrepController::~LightgrepController() {
  lg_destroy_pattern(ParsedPattern);
  lg_destroy_pattern_map(PatternInfo);
  lg_destroy_program(Prog);
}

LightgrepController& LightgrepController::Get() {
  static LightgrepController controller;
  return controller;
}

bool LightgrepController::addScanner(PatternScanner& scanner) {
  LG_Error* lgErr = 0;

  unsigned int patBegin = numeric_limits<unsigned int>::max(),
               patEnd = 0;

  int idx = -1;

  for (vector<const Handler*>::const_iterator h(scanner.handlers().begin()); h != scanner.handlers().end(); ++h) {
    bool good = false;
    if (lg_parse_pattern(ParsedPattern, (*h)->RE.c_str(), &(*h)->Options, &lgErr)) {
      for (vector<string>::const_iterator enc((*h)->Encodings.begin()); enc != (*h)->Encodings.end(); ++enc) {
        idx = lg_add_pattern(Fsm, PatternInfo, ParsedPattern, enc->c_str(), &lgErr);
        if (idx >= 0) {
          lg_pattern_info(PatternInfo, idx)->UserData = const_cast<void*>(static_cast<const void*>(&((*h)->Callback)));
          patBegin = std::min(patBegin, static_cast<unsigned int>(idx));
          good = true;
        }
      }
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
  scanner.patternRange() = make_pair(patBegin, patEnd);
  Scanners.push_back(&scanner);
  return true;
}

bool LightgrepController::addUserPatterns(PatternScanner& scanner, CallbackFnType* callbackPtr, const FindOptsStruct& user) {
  unsigned int patBegin = lg_pattern_map_size(PatternInfo),
               patEnd = 0;

  LG_KeyOptions opts;
  opts.FixedString = 0;
  opts.CaseInsensitive = 0;

  LG_Error *err = 0;

  for (vector<string>::const_iterator itr(user.Files.begin()); itr != user.Files.end(); ++itr) {
    ifstream file(itr->c_str(), ios::in);
    if (!file.is_open()) {
      cerr << "Could not open pattern file '" << *itr << "'." << endl;
      return false;
    }
    string contents = string(istreambuf_iterator<char>(file), istreambuf_iterator<char>());

    const char* contentsCStr = contents.c_str();
    if (lg_add_pattern_list(Fsm, PatternInfo, contentsCStr, DefaultEncodingsCStrings, 2, &opts, &err) < 0) {

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
  Prog = lg_create_program(Fsm, &progOpts);
  lg_destroy_fsm(Fsm);

  cerr << lg_pattern_map_size(PatternInfo) << " lightgrep patterns, logic size is " << lg_program_size(Prog) << " bytes, " << Scanners.size() << " active scanners" << std::endl;
}

struct HitData {
  LightgrepController* lgc;
  const vector<PatternScanner*>* scannerTable;
  const scanner_params* sp;
  const recursion_control_block* rcb;
};

void gotHit(void* userData, const LG_SearchHit* hit) {
  HitData* hd(static_cast<HitData*>(userData));
  hd->lgc->processHit(*hd->scannerTable, *hit, *hd->sp, *hd->rcb);
}

void LightgrepController::scan(const scanner_params& sp, const recursion_control_block &rcb) {
  if (!Prog) {
    // we had no valid patterns, do nothing
    return;
  }

  vector<PatternScanner*> scannerTable(lg_pattern_map_size(PatternInfo)); // [Keyword Index -> scanner], no ownership
  vector<PatternScanner*> scannerList;                                    // ownership list
  for (vector<PatternScanner*>::const_iterator itr(Scanners.begin()); itr != Scanners.end(); ++itr) {
    PatternScanner *s = (*itr)->clone();
    scannerList.push_back(s);
    for (unsigned int i = s->patternRange().first; i < s->patternRange().second; ++i) {
      scannerTable[i] = s;
    }
    s->initScan(sp);
  }
  LG_ContextOptions ctxOpts;
  ctxOpts.TraceBegin = 0xffffffffffffffff;
  ctxOpts.TraceEnd   = 0;

  LG_HCONTEXT ctx = lg_create_context(Prog, &ctxOpts);

  const sbuf_t &sbuf = sp.sbuf;

  HitData data = { this, &scannerTable, &sp, &rcb };

  // lg_search(ctx, (const char*)sbuf.buf, (const char*)sbuf.buf + sbuf.bufsize, 0, &data, gotHit);
  if (lg_search(ctx, (const char*)sbuf.buf, (const char*)sbuf.buf + sbuf.pagesize, 0, &data, gotHit) < numeric_limits<uint64_t>::max()) {
    lg_search_resolve(ctx, (const char*)sbuf.buf + sbuf.pagesize, (const char*)sbuf.buf + sbuf.bufsize, sbuf.pagesize, &data, gotHit);
  }
  lg_closeout_search(ctx, &data, gotHit);

  lg_destroy_context(ctx);

  // don't call PatternScanner::shutdown() on these! that only happens on prototypes
  for (vector<PatternScanner*>::const_iterator itr(scannerList.begin()); itr != scannerList.end(); ++itr) {
    (*itr)->finishScan(sp);
    delete *itr;
  }
}

void LightgrepController::processHit(const vector<PatternScanner*>& sTbl, const LG_SearchHit& hit, const scanner_params& sp, const recursion_control_block& rcb) {
  // lookup the handler's callback functor, then invoke it
  CallbackFnType* cbPtr(static_cast<CallbackFnType*>(lg_pattern_info(PatternInfo, hit.KeywordIndex)->UserData));
  ((*sTbl[hit.KeywordIndex]).*(*cbPtr))(hit, sp, rcb); // ...yep...
}

unsigned int LightgrepController::numPatterns() const {
  return lg_pattern_map_size(PatternInfo);
}

/*********************************************************/

void scan_lg(PatternScanner& scanner, const class scanner_params &sp, const recursion_control_block &rcb) {
  switch (sp.phase) {
  case scanner_params::PHASE_STARTUP:
    scanner.startup(sp);
    break;
  case scanner_params::PHASE_INIT:
    scanner.init(sp);
    if (!LightgrepController::Get().addScanner(scanner)) {
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

// class ExamplePatternScanner : public PatternScanner {
// public:
//   ExamplePatternScanner(): PatternScanner("lg_email"), EmailRecorder(0) {}
//   virtual ~ExamplePatternScanner() {}

//   virtual void init(const scanner_params& sp);
//   virtual void shutdown(const scanner_params&) {}

//   feature_recorder* EmailRecorder;

//   void defaultHitHandler(feature_recorder* fr, const LG_SearchHit& hit, const scanner_params& sp) {  
//     fr->write_buf(sp.sbuf, hit.Start, hit.End - hit.Start);
//   }

// private:
//   ExamplePatternScanner(const ExamplePatternScanner&);
//   ExamplePatternScanner& operator=(const ExamplePatternScanner&);
// };

// void ExamplePatternScanner::init(const scanner_params& sp) {
//   sp.info->name           = "ExamplePatternScanner";
//   sp.info->author         = "Simson L. Garfinkel";
//   sp.info->description    = "Scans for email addresses, domains, URLs, RFC822 headers, etc.";
//   sp.info->scanner_version= "1.0";

//   /* define the feature files this scanner created */
//   sp.info->feature_names.insert("email");
//   // sp.info->feature_names.insert("domain");
//   // sp.info->feature_names.insert("url");
//   // sp.info->feature_names.insert("rfc822");
//   // sp.info->feature_names.insert("ether");

//   /* define the histograms to make */
//   sp.info->histogram_defs.insert(histogram_def("email","","histogram",HistogramMaker::FLAG_LOWERCASE));
//   // sp.info->histogram_defs.insert(histogram_def("domain","","histogram"));
//   // sp.info->histogram_defs.insert(histogram_def("url","","histogram"));
//   // sp.info->histogram_defs.insert(histogram_def("url","://([^/]+)","services"));
//   // sp.info->histogram_defs.insert(histogram_def("url","://((cid-[0-9a-f])+[a-z.].live.com/)","microsoft-live"));
//   // sp.info->histogram_defs.insert(histogram_def("url","://[-_a-z0-9.]+facebook.com/.*[&?]{1}id=([0-9]+)","facebook-id"));
//   // sp.info->histogram_defs.insert(histogram_def("url","://[-_a-z0-9.]+facebook.com/([a-zA-Z0-9.]*[^/?&]$)","facebook-address",HistogramMaker::FLAG_LOWERCASE));
//   // sp.info->histogram_defs.insert(histogram_def("url","search.*[?&/;fF][pq]=([^&/]+)","searches"));

//   EmailRecorder = sp.fs.get_name("email");
// }

// ExamplePatternScanner EmailScanner;

// extern "C"
// void scan_lg_example(const class scanner_params &sp, const recursion_control_block &rcb) {
//   if (sp.phase == scanner_params::PHASE_STARTUP) {
//     cerr << "scan_lg_email - init" << endl;
//     EmailScanner.init(sp);
//     LightgrepController::Get().addScanner(EmailScanner);
//   }
//   else if (sp.phase == scanner_params::PHASE_SHUTDOWN) {
//     cerr << "scan_lg_email - cleanup" << endl;
//     EmailScanner.cleanup(sp);
//   }
// }


// const vector<string> DefaultEncodings(&DefaultEncodingsCStrings[0], &DefaultEncodingsCStrings[2]);

// Handler EmailAddr(EmailScanner, "jon@lightbox", DefaultEncodings, bind(&ExamplePatternScanner::defaultHitHandler, &EmailScanner, EmailScanner.EmailRecorder, _1, _2));

#endif // HAVE_LIBLIGHTGREP
