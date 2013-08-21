#include "config.h"

// if liblightgrep isn't present, compiles to nothing
#ifdef HAVE_LIBLIGHTGREP

#include "be13_api/bulk_extractor_i.h"
#include "beregex.h"
#include "histogram.h"

#include <lightgrep/api.h>

#include <algorithm>
#include <iostream>
#include <iterator>
#include <numeric>
#include <sstream>

using namespace std;

extern regex_list find_list;

namespace { // local namespace hides these from other translation units

  static LG_HPROGRAM    lgProgram;
  static LG_HPATTERNMAP lgPatternMap;

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

    vector<string> patterns(makePatterns()),
                   encodings(makeEncodings());

    if (patterns.empty()) {
      return;
    }

    // par for bytes per char in patterns, all encodings
    const unsigned bytesPerCharEstimate = 4;

    LG_HPATTERN pattern = lg_create_pattern();
    LG_HFSM fsm = lg_create_fsm(bytesPerCharEstimate*totalChars(patterns));

    lgPatternMap = lg_create_pattern_map(patterns.size() * encodings.size());

    LG_Error* lgErr = 0;

    LG_KeyOptions opts;
    opts.FixedString = 0;
    opts.CaseInsensitive = 0;

    // parse cross-product of patterns & encodings
    vector<string>::const_iterator pat(patterns.begin());
    const vector<string>::const_iterator pend(patterns.end());

    vector<string>::const_iterator enc;
    const vector<string>::const_iterator eend(encodings.end());

    for ( ; pat != pend; ++pat) {
      if (lg_parse_pattern(pattern, pat->c_str(), &opts, &lgErr)) {
        for (enc = encodings.begin(); enc != eend; ++enc) {
          if (0 > lg_add_pattern(fsm, lgPatternMap, pattern, enc->c_str(), &lgErr)) {
            handleError(pattern, fsm, lgErr);
            return;
          }
        }
      }
      else {
        handleError(pattern, fsm, lgErr);
        return;      
      }
    }

    // turn the parsed patterns into efficient bytecode representation
    LG_ProgramOptions progOpts;
    progOpts.Determinize = 1;

    lgProgram = lg_create_program(fsm, &progOpts);
    cleanup(pattern, fsm);
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
    init(sp);
  }
  else if (sp.phase == scanner_params::PHASE_SHUTDOWN) {
    // cleanup heap objects in static handles
    lg_destroy_program(lgProgram);
    lg_destroy_pattern_map(lgPatternMap);
  }
  else if (sp.phase == scanner_params::PHASE_SCAN) {
    if (!(lgPatternMap && lg_pattern_map_size(lgPatternMap) && lgProgram)) {
      return;
    }
    // cout << "lightgrep scanning..." << endl;
    LG_ContextOptions ctxOpts;
    // these are debugging options that we'll remove
    ctxOpts.TraceBegin = 0xffffffffffffffff;
    ctxOpts.TraceEnd   = 0;

    // A context maintains state related to searching a particular stream,
    // built from the program. If BE maintained scanner state for different
    // threads, then contexts could be reused, which would be a slight win.
    LG_HCONTEXT ctx = lg_create_context(lgProgram, &ctxOpts);

    const sbuf_t &sbuf = sp.sbuf;

    HitData data(sp.fs.get_name("lightgrep"), &sbuf);

    // search the buffer all in one go
    lg_search(ctx, (const char*)sbuf.buf, (const char*)sbuf.buf + sbuf.bufsize, 0, &data, gotHit);
    // if there are any hits at the end of the buffer that could extend with
    // more data, lg_closeout_search will flush them out
    lg_closeout_search(ctx, &data, gotHit);

    // cleanup
    lg_destroy_context(ctx);

    // cout << data.Count << " hits on block " << sbuf.pos0 << endl;
  }
}

#endif // HAVE_LIBLIGHTGREP
