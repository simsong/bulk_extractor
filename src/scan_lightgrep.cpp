#include "config.h"

// if liblightgrep isn't present, compiles to nothing
#ifdef HAVE_LIBLIGHTGREP

#include "be13_api/bulk_extractor_i.h"
#include "be13_api/beregex.h"
#include "histogram.h"
#include "pattern_scanner.h"

#include <lightgrep/api.h>

#include <algorithm>
#include <iostream>
#include <iterator>
#include <numeric>

using namespace std;

namespace { // local namespace hides these from other translation units

  class FindScanner: public PatternScanner {
  public:
    FindScanner(): PatternScanner("lightgrep"), LgRec(0) {}
    virtual ~FindScanner() {}

    virtual FindScanner* clone() const {
      return new FindScanner(*this);
    };

    virtual void startup(const scanner_params& sp) {
      sp.info->name            = name();
      sp.info->author          = "Jon Stewart";
      sp.info->description     = "Advanced search for patterns";
      sp.info->scanner_version = "0.2";
      sp.info->flags           = scanner_info::SCANNER_FIND_SCANNER |
                                 scanner_info::SCANNER_FAST_FIND;
      sp.info->feature_names.insert(name());
      sp.info->histogram_defs.insert(histogram_def(
       name(), "", "histogram", HistogramMaker::FLAG_LOWERCASE));
    }

    virtual void init(const scanner_params& sp) {
    }

    virtual void initScan(const scanner_params& sp) {
      LgRec = sp.fs.get_name(name());
    }

    feature_recorder* LgRec;

    void processHit(const LG_SearchHit& hit, const scanner_params& sp, const recursion_control_block& rcb) {
      LgRec->write_buf(sp.sbuf, hit.Start, hit.End - hit.Start);
    }

  private:
    FindScanner(const FindScanner& x): PatternScanner(x), LgRec(x.LgRec) {}

    FindScanner& operator=(const FindScanner&);
  };

  FindScanner Scanner;

  CallbackFnType ProcessHit;
}

extern "C"
void scan_lightgrep(const class scanner_params &sp, const recursion_control_block &rcb) {
  switch (sp.phase) {
  case scanner_params::PHASE_STARTUP:
    Scanner.startup(sp);
    ProcessHit = static_cast<CallbackFnType>(&FindScanner::processHit);
    break;
  case scanner_params::PHASE_INIT:
    {
      Scanner.init(sp);
      LightgrepController& lg(LightgrepController::Get());
      lg.addUserPatterns(Scanner, &ProcessHit, FindOpts::get());
      lg.regcomp();
      break;
    }
  case scanner_params::PHASE_SCAN:
    LightgrepController::Get().scan(sp, rcb);
    break;
  case scanner_params::PHASE_SHUTDOWN:
    Scanner.shutdown(sp);
    break;
  default:
    break;
  }
}

#endif // HAVE_LIBLIGHTGREP
