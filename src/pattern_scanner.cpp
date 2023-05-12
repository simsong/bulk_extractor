#include "config.h"

// if liblightgrep isn't present, compiles to nothing
#ifdef HAVE_LIBLIGHTGREP

#include "pattern_scanner.h"
#include "scanner_set.h"

#include <lightgrep/api.h>

#ifdef LGBENCHMARK
#include <chrono>
#endif

namespace {
  const char* DefaultEncodingsCStrings[] = {"UTF-8", "UTF-16LE"};
  const unsigned int NumDefaultEncodings = 2;
}

void PatternScanner::shutdown(const scanner_params&) {
}

struct LgContextHolder {
  LG_HCONTEXT Ctx;

  LgContextHolder(LG_HPROGRAM Prog, LG_ContextOptions* ctxOpts) {Ctx = lg_create_context(Prog, ctxOpts);}
  ~LgContextHolder() {lg_destroy_context(Ctx);}

};

LightgrepController::LightgrepController()
: ParsedPattern(lg_create_pattern()),       // Reuse the parsed pattern data structure for efficiency
  Fsm(lg_create_fsm(1000, 1 << 20)),              // Reserve space for 1M states in the automaton--will grow if needed
  Prog(0),
  Scanners()
{
}

LightgrepController::~LightgrepController() {
  lg_destroy_pattern(ParsedPattern);
  lg_destroy_program(Prog);
  if (Fsm) {
    lg_destroy_fsm(Fsm);
    Fsm = 0;
  }
}

bool LightgrepController::addUserPatterns(
  PatternScanner& scanner, 
  const vector<string>& cli_patterns, 
  const vector<filesystem::path>& user_files) {

  LG_Error *err = 0;
  LG_KeyOptions opts;
  opts.FixedString = 0;
  opts.CaseInsensitive = 0;

  bool good = true;

  // add patterns from single command-line arguments
  for (const auto& itr : cli_patterns) {
    if (lg_parse_pattern(ParsedPattern, itr.c_str(), &opts, &err)) {
      for (unsigned int i = 0; i < NumDefaultEncodings; ++i) {
        if (lg_add_pattern(Fsm, ParsedPattern, DefaultEncodingsCStrings[i], 0, &err) < 0) {
          good = false;
          break;
        }
      }
    } else {
      good = false;
    }
    if (!good) {
      cerr << "Lightgrep error parsing '" << itr.c_str() << "': " << err->Message << endl;
      lg_free_error(err);
      return false;
    }
  }

  // Add patterns from files
  for (const auto& itr : user_files) {
    ifstream file(itr.c_str(), ios::in);
    if (!file.is_open()) {
      cerr << "Lightgrep scanner could not open pattern file '" << itr.c_str() << "'." << endl;
      return false;
    }
    string contents = string(istreambuf_iterator<char>(file), istreambuf_iterator<char>());

    const char* contentsCStr = contents.c_str();
    // Add all the patterns from the files in one fell swoop
    if (lg_add_pattern_list(Fsm, contentsCStr, itr.c_str(), DefaultEncodingsCStrings, NumDefaultEncodings, &opts, &err) < 0) {
      vector<string> lines;
      istringstream input(contents);
      string line;
      while (input) {
        getline(input, line);
        lines.push_back(line);
      }
      LG_Error* cur(err);
      while (cur) {
        cerr << "Lightgrep parsing error in " << itr.c_str() << ", on line " << cur->Index+1 << ", on pattern '" << lines[cur->Index]
          << "': " << cur->Message << endl;
        cur = cur->Next;
      }
      lg_free_error(err);
      return false;
    }
  }

  return true;
}

void LightgrepController::regcomp() {
  LG_ProgramOptions progOpts;
  progOpts.DeterminizeDepth = 10;
  // Create an optimized, immutable form of the accumulated automaton
  Prog = lg_create_program(Fsm, &progOpts);
  lg_destroy_fsm(Fsm);
  Fsm = 0;

  #ifdef LGBENCHMARK
  cerr << "timer second ratio " << chrono::high_resolution_clock::period::num << "/" <<
    chrono::high_resolution_clock::period::den << endl;
  #endif
}

struct HitData {
  feature_recorder &recorder;
  const sbuf_t &sbuf;
};

void gotHit(void* userData, const LG_SearchHit* hit) {
  #ifdef LGBENCHMARK
  // no callback, just increment hit counter
  ++(*static_cast<uint64_t*>(userData));
  #else
  HitData* data(reinterpret_cast<HitData*>(userData));
  data->recorder.write_buf(data->sbuf, hit->Start, hit->End - hit->Start);
  #endif
}

void LightgrepController::scan(const scanner_params& sp) {
  // Scan the sbuf for pattern hits
  if (!Prog) {
    // we had no valid patterns, do nothing
    return;
  }

  LG_ContextOptions ctxOpts;
  ctxOpts.TraceBegin = 0xffffffffffffffff;
  ctxOpts.TraceEnd   = 0;
  thread_local LgContextHolder ctx(Prog, &ctxOpts);
  lg_reset_context(ctx.Ctx);
  //LG_HCONTEXT ctx = lg_create_context(Prog, &ctxOpts); // create a search context; cannot be shared, so local to scan

  const sbuf_t &sbuf = *sp.sbuf;
  HitData callbackInfo = { sp.named_feature_recorder("lightgrep"), *sp.sbuf };
  void*   userData = &callbackInfo;

  #ifdef LGBENCHMARK // perform timings of lightgrep search functions only -- no callbacks
  uint64_t hitCount = 0;
  userData = &hitCount; // switch things out for a counter

  auto startClock = std::chrono::high_resolution_clock::now();
  // std::cout << "Starting block " << sbuf.pos0.str() << std::endl;
  #endif

  // search the sbuf in one go
  // the gotHit() function will be invoked for each pattern hit
  if (lg_search(ctx.Ctx, (const char*)sbuf.get_buf(), (const char*)sbuf.get_buf() + sbuf.pagesize, 0, userData, gotHit) < numeric_limits<uint64_t>::max()) {
    // resolve potential hits that want data into the sbuf margin, without beginning any new hits
    lg_search_resolve(ctx.Ctx, (const char*)sbuf.get_buf() + sbuf.pagesize, (const char*)sbuf.get_buf() + sbuf.bufsize, sbuf.pagesize, userData, gotHit);
  }
  // flush any remaining hits; there's no more data
  lg_closeout_search(ctx.Ctx, userData, gotHit);

  #ifdef LGBENCHMARK
  auto endClock = std::chrono::high_resolution_clock::now();
  auto t = endClock - startClock;
  double seconds = double(t.count() * chrono::high_resolution_clock::period::num) / chrono::high_resolution_clock::period::den;
  double bw = double(sbuf.pagesize) / (seconds * 1024 * 1024);
  std::stringstream buf;
  buf << " ** Time: " << sbuf.pos0.str() << '\t' << sbuf.pagesize << '\t' << t.count() << '\t' << seconds<< '\t' << hitCount << '\t' << bw << std::endl;
  std::cout << buf.str();
  #endif

  //lg_destroy_context(ctx);
}

unsigned int LightgrepController::numPatterns() const {
  return Prog ? lg_prog_pattern_count(Prog) : 0;
}

#endif // HAVE_LIBLIGHTGREP
