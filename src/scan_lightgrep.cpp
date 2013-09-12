#include "config.h"

// if liblightgrep isn't present, compiles to nothing
#ifdef HAVE_LIBLIGHTGREP

#include "be13_api/bulk_extractor_i.h"
#include "beregex.h"
#include "histogram.h"
#include "pattern_scanner.h"

#include <lightgrep/api.h>

#include <algorithm>
#include <iostream>
#include <iterator>
#include <numeric>
#include <sstream>

using namespace std;

extern regex_list find_list;

namespace { // local namespace hides these from other translation units

  string beregex_pat(const beregex* ber) { return ber->pat; }

  vector<string> makePatterns() {
    // get string from beregex list
    vector<string> ret;

    transform(
      find_list.patterns.begin(), find_list.patterns.end(),
      back_inserter(ret), beregex_pat
    );

    return ret;
  }

  vector<string> makeEncodings() {
    vector<string> ret;
    ret.push_back("UTF-8");
    ret.push_back("UTF-16LE");
    return ret;
  }

  ssize_t slen(ssize_t count, const string& str) {
    return count + str.size();
  }

  ssize_t totalChars(const vector<string>& strs) {
    return accumulate(strs.begin(), strs.end(), 0, slen);
  }

  void cleanup(LG_HPATTERN pat, LG_HFSM fsm) {
    lg_destroy_pattern(pat);
    lg_destroy_fsm(fsm);
  }

  void handleError(LG_HPATTERN pat, LG_HFSM fsm, LG_Error* lgErr) {
    cerr << lgErr->Message << endl;
    cleanup(pat, fsm);
    lg_free_error(lgErr);
  }

  void init(const scanner_params& sp) {
    sp.info->name            = "lightgrep";
    sp.info->author          = "Jon Stewart";
    sp.info->description     = "Advaned search for patterns";
    sp.info->scanner_version = "0.1";
    sp.info->flags	         = scanner_info::SCANNER_FIND_SCANNER |
                               scanner_info::SCANNER_FAST_FIND;
    sp.info->feature_names.insert("lightgrep");
    sp.info->histogram_defs.insert(histogram_def(
      "lightgrep", "", "histogram", HistogramMaker::FLAG_LOWERCASE));
  }

  struct HitData {
    feature_recorder* Fr;
    const sbuf_t*     Sbuf;
    uint64_t          Count;

    HitData(feature_recorder* f, const sbuf_t* sb): Fr(f), Sbuf(sb), Count(0) {}
  };

  void gotHit(void* userData, const LG_SearchHit* hit) {
    // hit callback
    HitData* data = reinterpret_cast<HitData*>(userData);

    data->Fr->write_buf(*data->Sbuf, hit->Start, hit->End - hit->Start);
    ++data->Count;
  }
}

extern "C"
void scan_lightgrep(const class scanner_params &sp, const recursion_control_block &rcb) {
  if (sp.phase == scanner_params::PHASE_STARTUP) {
    // check to see whether there's a keywords file
    // if so, load into FSM
    init(sp);

    LightgrepController& lg(LightgrepController::Get());
    lg.regcomp();
    std::cout << "Lightgrep will look for a total of " << lg.numPatterns() << " patterns" << std::endl;
  }
  else if (sp.phase == scanner_params::PHASE_SHUTDOWN) {
    // meh
  }
  else if (sp.phase == scanner_params::PHASE_SCAN) {
    LightgrepController& lg(LightgrepController::Get());
    lg.scan(sp, rcb);
  }
}

#endif // HAVE_LIBLIGHTGREP
