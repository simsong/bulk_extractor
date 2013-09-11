#include "config.h"

// if liblightgrep isn't present, compiles to nothing
#ifdef HAVE_LIBLIGHTGREP

#include <string>

#include "be13_api/bulk_extractor_i.h"
#include "histogram.h"
#include "pattern_scanner.h"

namespace base16 {
  //
  // the scanner
  //

  const int BASE16_INVALID = -1;
  const int BASE16_IGNORE = -2;

  class Scanner: public PatternScanner {
  public:
    Scanner(): PatternScanner("base16_lg"), Recorder(0) {}
    virtual ~Scanner() {}

    virtual void init(const scanner_params& sp);
    virtual void cleanup(const scanner_params&) {}

    feature_recorder* Recorder;

    void hitHandler(const LG_SearchHit& hit, const scanner_params& sp, const recursion_control_block& rcb) {
      decode(sp.sbuf, hit.Start + 1, hit.End - hit.Start - 1, sp, rcb);
    }

  private:
    Scanner(const Scanner&);
    Scanner& operator=(const Scanner&);

    void decode(const sbuf_t& osbuf, size_t pos, size_t len, const scanner_params& sp, const recursion_control_block& rcb);

    int base16array[256];
  };

  void Scanner::init(const scanner_params& sp) {
    assert(sp.sp_version == scanner_params::CURRENT_SP_VERSION);
    assert(sp.info->si_version == scanner_info::CURRENT_SI_VERSION);

    sp.info->name             = "base16";
    sp.info->author           = "Simson L. Garfinkel";
    sp.info->description      = "Base16 (hex) scanner";
    sp.info->scanner_version  = "1.0";
    sp.info->flags            = scanner_info::SCANNER_RECURSE;

    sp.info->feature_names.insert("hex"); // notable hex values

// FIXME: Convert this to a constant array?
    // Create the base16 array
    static const u_char *ignore_string = (const u_char *)"\r\n \t";

    for (int i = 0; i < 256; ++i) {
      base16array[i] = BASE16_INVALID;
    }

    for(const u_char *ch = ignore_string; *ch ; ++ch) {
      base16array[(int)*ch] = BASE16_IGNORE;
    }
    for (int ch='A'; ch<='F'; ++ch) { base16array[ch] = ch-'A'+10; }
    for (int ch='a'; ch<='f'; ++ch) { base16array[ch] = ch-'a'+10; }
    for (int ch='0'; ch<='9'; ++ch) { base16array[ch] = ch-'0'; }
  }

  Scanner TheScanner;

  const char* const DefaultEncodingsCStrings[] = {"UTF-8", "UTF-16LE"};

  const vector<string> DefaultEncodings(
    &DefaultEncodingsCStrings[0], &DefaultEncodingsCStrings[2]
  );

  // Don't re-analyze hex bufs smaller than this
  const unsigned int opt_min_hex_buf = 64;

  void Scanner::decode(const sbuf_t& osbuf, size_t pos, size_t len, const scanner_params& sp, const recursion_control_block& rcb) {
    sbuf_t sbuf(osbuf, pos, len);  // the substring we are working with

    CharClass cct;
    managed_malloc<uint8_t>b(sbuf.pagesize/2);
    if (b.buf == 0) return;

    size_t p = 0;
    // First get the characters
    for (size_t i = 0; i+1 < sbuf.pagesize; ) {
      // stats on the new characters

      // decode the two characters
      const int msb = base16array[sbuf[i]];
      if (msb == BASE16_IGNORE || msb == BASE16_INVALID) {
        i++;          // This character not valid
        continue;
      }

      assert(msb >= 0 && msb < 16);
      const int lsb = base16array[sbuf[i+1]];
      if (lsb == BASE16_IGNORE || lsb == BASE16_INVALID) {
        // If first char is valid hex and second isn't, this isn't hex
        return;
      }

      assert(lsb >= 0 && lsb < 16);
      b.buf[p++] = (msb<<4) | lsb;
      i+=2;
    }

    if (cct.range_0_9 == 0 || cct.range_A_Fi == 0){
      return;   // we need 0-9 and A-F
    }

    // Alert on byte sequences of 48, 128 or 256 bits
    if (p == 48/8 || p == 128/8 || p == 256/8) {
      // it validates; write original with context
      Recorder->write_buf(osbuf, pos, len);
      return; // Small keys don't get recursively analyzed
    }

    if (p > opt_min_hex_buf){
      sbuf_t nsbuf(sbuf.pos0, b.buf, p, p, false);
      (*rcb.callback)(scanner_params(sp, nsbuf)); // recurse
    }
  }

  //
  // patterns
  //

  // FIXME: trailing context
  // FIXME: leading context
  /* 
   * hex with junk before it.
   * {0,4} means we have 0-4 space characters
   * {6,}  means minimum of 6 hex bytes
   */
//  const std::string HEX("[^0-9A-F]([0-9A-F]{2}[ \\t\\n\\r]{0,4}){6,}[^0-9A-F]");

  Handler HEX(
    TheScanner,
    "[^0-9A-F]([0-9A-F]{2}[ \\t\\n\\r]{0,4}){6,}[^0-9A-F]",
    DefaultEncodings,
    bind(&Scanner::hitHandler, &TheScanner, _1, _2, _3)
  );
}

extern "C"
void scan_base16_lg(const class scanner_params &sp, const recursion_control_block &rcb) {
  using namespace base16;

  if (sp.phase == scanner_params::PHASE_STARTUP) {
    cerr << "scan_base16_lg - init" << endl;
    TheScanner.init(sp);
    LightgrepController::Get().addScanner(TheScanner);
  }
  else if (sp.phase == scanner_params::PHASE_SHUTDOWN) {
    cerr << "scan_base16_lg - cleanup" << endl;
    TheScanner.cleanup(sp);
  }
}

#endif // HAVE_LIBLIGHTGREP
