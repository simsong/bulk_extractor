#include "config.h"

// if liblightgrep isn't present, compiles to nothing
#ifdef HAVE_LIBLIGHTGREP

#include <algorithm>
#include <iostream>
#include <iterator>
#include <numeric>

#include "be20_api/scanner_params.h"

//#include "be20_api/beregex.h"

#include "lightgrep_controller.h"
// #include <lightgrep/api.h>

namespace {
}

extern "C"
void scan_lightgrep(struct scanner_params &sp) {
    static LightgrepController LG;

    switch (sp.phase) {
    case scanner_params::PHASE_INIT:
        std::cerr << "scan_lightgrep Init called\n";
        {
            sp.info->set_name("lightgrep");
            sp.info->name       = "lightgrep";
            sp.info->author         = "Jon Stewart";
            sp.info->description    = "Search for multiple regular expressions in different encodings";
            sp.info->scanner_version= "1.1";
            sp.info->scanner_flags.find_scanner = true; // this is a find scanner
            sp.info->feature_defs.push_back( feature_recorder_def("lightgrep"));
            auto lowercase = histogram_def::flags_t(); lowercase.lowercase = true;
            sp.info->histogram_defs.push_back( histogram_def("lightgrep", "lightgrep", "", "","histogram", lowercase));
            break;
        }
    case scanner_params::PHASE_INIT2:
        {
            std::cerr << "scan_lightgrep Init2 called\n";
            LG.setup();
          // Scanner.init(sp);
          // LightgrepController& lg(LightgrepController::Get());
          // lg.addUserPatterns(Scanner, &ProcessHit, sp.ss->sc); // note: FindOpts now passed in ScannerConfig
          // lg.regcomp();
            break;
        }
    case scanner_params::PHASE_SCAN:
        std::cerr << "scan_lightgrep Scan called on " << sp.sbuf->pos0 << "\n";
        // LightgrepController::Get().scan(sp);
        break;
    case scanner_params::PHASE_SHUTDOWN:
        std::cerr << "scan_lightgrep Shutdown called\n";
        // Scanner.shutdown(sp);
        LG.teardown();
        break;
    default:
        break;
    }
}

#endif // HAVE_LIBLIGHTGREP
