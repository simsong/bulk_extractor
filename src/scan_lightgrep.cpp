#include "config.h"

// if liblightgrep isn't present, compiles to nothing
#ifdef HAVE_LIBLIGHTGREP

#include <algorithm>
#include <iostream>
#include <iterator>
#include <numeric>

#include "be20_api/scanner_params.h"
#include "be20_api/scanner_set.h"
#include "be20_api/histogram_def.h"
#include "pattern_scanner.h"

#include <lightgrep/api.h>


namespace { // local namespace hides these from other translation units

  class FindScanner: public PatternScanner {
  public:
    FindScanner(): PatternScanner("lightgrep"), LgRec(0) {}
    virtual ~FindScanner() {}

    virtual FindScanner* clone() const {
      return new FindScanner(*this);
    };

    virtual void startup(const scanner_params& sp) {
        sp.info->set_name("scan_lightgrep");
        sp.info->author          = "Jon Stewart";
        sp.info->description     = "Advanced search for patterns";
        sp.info->scanner_version = "1.0";
        sp.info->feature_defs.push_back( feature_recorder_def("lightgrep"));
        sp.info->scanner_flags.find_scanner = true;
        auto lowercase = histogram_def::flags_t(); 
        lowercase.lowercase = true;
        sp.info->histogram_defs.push_back(histogram_def(name(), name(), "", "", "histogram", lowercase));
    }

    virtual void init(const scanner_params& sp) {
    }

    feature_recorder* LgRec;

  private:
    FindScanner(const FindScanner& x): PatternScanner(x), LgRec(x.LgRec) {}

    FindScanner& operator=(const FindScanner&);
  };
}

extern "C"
void scan_lightgrep(struct scanner_params &sp) {
  static std::unique_ptr<FindScanner> lg_findscanner_ptr;
  static std::unique_ptr<LightgrepController> lg_ptr;
  switch (sp.phase) {
  case scanner_params::PHASE_INIT:
    lg_findscanner_ptr.reset(new FindScanner);
    lg_findscanner_ptr->startup(sp);
    break;
  case scanner_params::PHASE_INIT2:
    {
      lg_findscanner_ptr->init(sp);
      lg_ptr.reset(new LightgrepController);
      lg_ptr->addUserPatterns(*lg_findscanner_ptr, sp.ss->find_patterns(), sp.ss->find_files());
      lg_ptr->regcomp();
    }
    break;
  case scanner_params::PHASE_SCAN:
    lg_ptr->scan(sp);
    break;
  case scanner_params::PHASE_SHUTDOWN:
    lg_findscanner_ptr.reset();
    lg_ptr.reset();
    break;
  default:
    break;
  }
}

#endif // HAVE_LIBLIGHTGREP
