#include "config.h"

// if liblightgrep isn't present, compiles to nothing
#ifdef HAVE_LIBLIGHTGREP

#include <algorithm>
#include <iostream>
#include <iterator>
#include <numeric>

#include "be20_api/scanner_params.h"

//#include "be20_api/beregex.h"

#include <lightgrep/api.h>


// namespace { // local namespace hides these from other translation units

//   class FindScanner: public PatternScanner {
//   public:
//     FindScanner(): PatternScanner("lightgrep"), LgRec(0) {}
//     virtual ~FindScanner() {}

//     virtual FindScanner* clone() const {
//       return new FindScanner(*this);
//     };

//     virtual void startup(const scanner_params& sp) {
//         sp.info.set_name("scan_lightgrep");
//         sp.info->author          = "Jon Stewart";
//         sp.info->description     = "Advanced search for patterns";
//         sp.info->scanner_version = "0.2";
//         sp.info->flags           = scanner_info::SCANNER_FIND_SCANNER | scanner_info::SCANNER_FAST_FIND;
//         sp.info->feature_names.insert(name());
//         sp.info->histogram_defs.insert(histogram_def( name(), "", "histogram", HistogramMaker::FLAG_LOWERCASE));
//     }

//     virtual void init(const scanner_params& sp) {
//     }

//     virtual void initScan(const scanner_params& sp) {
//       LgRec = &sp.named_feature_recorder(name());
//     }

//     feature_recorder* LgRec;

//     void processHit(const LG_SearchHit& hit, const scanner_params& sp) {
//       LgRec->write_buf(sp.sbuf, hit.Start, hit.End - hit.Start);
//     }

//   private:
//     FindScanner(const FindScanner& x): PatternScanner(x), LgRec(x.LgRec) {}

//     FindScanner& operator=(const FindScanner&);
//   };

//   FindScanner Scanner;

//   CallbackFnType ProcessHit;
// }

extern "C"
void scan_lightgrep(struct scanner_params &sp) {
    switch (sp.phase) {
    case scanner_params::PHASE_INIT:
        // Scanner.startup(sp);
        // ProcessHit = static_cast<CallbackFnType>(&FindScanner::processHit);
        break;
    case scanner_params::PHASE_INIT2:
        {
          // Scanner.init(sp);
          // LightgrepController& lg(LightgrepController::Get());
          // lg.addUserPatterns(Scanner, &ProcessHit, sp.ss->sc); // note: FindOpts now passed in ScannerConfig
          // lg.regcomp();
          break;
        }
    case scanner_params::PHASE_SCAN:
        // LightgrepController::Get().scan(sp);
        break;
    case scanner_params::PHASE_SHUTDOWN:
        // Scanner.shutdown(sp);
        break;
    default:
        break;
    }
}

#endif // HAVE_LIBLIGHTGREP
