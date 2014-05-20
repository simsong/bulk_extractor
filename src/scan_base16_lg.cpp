#include "config.h"

// if liblightgrep isn't present, compiles to nothing
#ifdef HAVE_LIBLIGHTGREP

#include <string>

#include "be13_api/bulk_extractor_i.h"
#include "histogram.h"
#include "pattern_scanner.h"

namespace base16 {
//  const char* const DefaultEncodingsCStrings[] = {"UTF-8", "UTF-16LE"};
  const char* const DefaultEncodingsCStrings[] = {"UTF-8"};

  const vector<string> DefaultEncodings(
    DefaultEncodingsCStrings,
    DefaultEncodingsCStrings +
      sizeof(DefaultEncodingsCStrings)/sizeof(DefaultEncodingsCStrings[0])
  );

  const LG_KeyOptions DefaultOptions = { 0, 1 }; // patterns, case-insensitive

  //
  // the scanner
  //

  class Scanner: public PatternScanner {
  public:
    Scanner(): PatternScanner("base16_lg"), Recorder(0) {}
    virtual ~Scanner() {}

    virtual Scanner* clone() const { return new Scanner(*this); }

    virtual void startup(const scanner_params& sp);
    virtual void init(const scanner_params& sp);
    virtual void initScan(const scanner_params&);

    feature_recorder* Recorder;

    void hitHandler(const LG_SearchHit& hit, const scanner_params& sp, const recursion_control_block& rcb) {
      decode(sp.sbuf, hit.Start, hit.End - hit.Start, sp, rcb);
    }

  private:
    Scanner(const Scanner& s):
      PatternScanner(s),
      Recorder(s.Recorder)
    {}

    Scanner& operator=(const Scanner&);

    void decode(const sbuf_t& osbuf, size_t pos, size_t len, const scanner_params& sp, const recursion_control_block& rcb);
  };

  const uint16_t BASE16_LSN[256] = {
    256, 256, 256, 256, 256, 256, 256, 256,
    256, 256, 256, 256, 256, 256, 256, 256,
    256, 256, 256, 256, 256, 256, 256, 256,
    256, 256, 256, 256, 256, 256, 256, 256,
    256, 256, 256, 256, 256, 256, 256, 256,
    256, 256, 256, 256, 256, 256, 256, 256,
      0,   1,   2,   3,   4,   5,   6,   7,
      8,   9, 256, 256, 256, 256, 256, 256,
    256,  10,  11,  12,  13,  14,  15, 256,
    256, 256, 256, 256, 256, 256, 256, 256,
    256, 256, 256, 256, 256, 256, 256, 256,
    256, 256, 256, 256, 256, 256, 256, 256,
    256,  10,  11,  12,  13,  14,  15, 256,
    256, 256, 256, 256, 256, 256, 256, 256,
    256, 256, 256, 256, 256, 256, 256, 256,
    256, 256, 256, 256, 256, 256, 256, 256,
    256, 256, 256, 256, 256, 256, 256, 256,
    256, 256, 256, 256, 256, 256, 256, 256,
    256, 256, 256, 256, 256, 256, 256, 256,
    256, 256, 256, 256, 256, 256, 256, 256,
    256, 256, 256, 256, 256, 256, 256, 256,
    256, 256, 256, 256, 256, 256, 256, 256,
    256, 256, 256, 256, 256, 256, 256, 256,
    256, 256, 256, 256, 256, 256, 256, 256,
    256, 256, 256, 256, 256, 256, 256, 256,
    256, 256, 256, 256, 256, 256, 256, 256,
    256, 256, 256, 256, 256, 256, 256, 256,
    256, 256, 256, 256, 256, 256, 256, 256,
    256, 256, 256, 256, 256, 256, 256, 256,
    256, 256, 256, 256, 256, 256, 256, 256,
    256, 256, 256, 256, 256, 256, 256, 256,
    256, 256, 256, 256, 256, 256, 256, 256
  };

  const uint16_t BASE16_MSN[256] = {
    256, 256, 256, 256, 256, 256, 256, 256,
    256, 256, 256, 256, 256, 256, 256, 256,
    256, 256, 256, 256, 256, 256, 256, 256,
    256, 256, 256, 256, 256, 256, 256, 256,
    256, 256, 256, 256, 256, 256, 256, 256,
    256, 256, 256, 256, 256, 256, 256, 256,
      0,  16,  32,  48,  64,  80,  96, 112,
    128, 144, 256, 256, 256, 256, 256, 256,
    256, 160, 176, 192, 208, 224, 240, 256,
    256, 256, 256, 256, 256, 256, 256, 256,
    256, 256, 256, 256, 256, 256, 256, 256,
    256, 256, 256, 256, 256, 256, 256, 256,
    256, 160, 176, 192, 208, 224, 240, 256,
    256, 256, 256, 256, 256, 256, 256, 256,
    256, 256, 256, 256, 256, 256, 256, 256,
    256, 256, 256, 256, 256, 256, 256, 256,
    256, 256, 256, 256, 256, 256, 256, 256,
    256, 256, 256, 256, 256, 256, 256, 256,
    256, 256, 256, 256, 256, 256, 256, 256,
    256, 256, 256, 256, 256, 256, 256, 256,
    256, 256, 256, 256, 256, 256, 256, 256,
    256, 256, 256, 256, 256, 556, 256, 256,
    256, 256, 256, 256, 256, 256, 256, 256,
    256, 256, 256, 256, 256, 256, 256, 256,
    256, 256, 256, 256, 256, 256, 256, 256,
    256, 256, 256, 256, 256, 256, 256, 256,
    256, 256, 256, 256, 256, 256, 256, 256,
    256, 256, 256, 256, 256, 256, 256, 256,
    256, 256, 256, 256, 256, 256, 256, 256,
    256, 256, 256, 256, 256, 256, 256, 256,
    256, 256, 256, 256, 256, 256, 256, 256,
    256, 256, 256, 256, 256, 256, 256, 256
  };

  void Scanner::startup(const scanner_params& sp) {
    assert(sp.sp_version == scanner_params::CURRENT_SP_VERSION);
    assert(sp.info->si_version == scanner_info::CURRENT_SI_VERSION);

    sp.info->name             = "base16_lg";
    sp.info->author           = "Simson L. Garfinkel";
    sp.info->description      = "Base16 (hex) scanner";
    sp.info->scanner_version  = "1.0";
    sp.info->flags            = scanner_info::SCANNER_RECURSE;

    sp.info->feature_names.insert("hex"); // notable hex values
  }

  void Scanner::init(const scanner_params& sp) {
    //
    // patterns
    //

    /*
     * a hex string
     * {0,2} means we have 0-2 space characters
     * {6,}  means minimum of 6 hex bytes
     */
    const std::string HEX("[0-9A-F]{2}(([ \\n]|\\r\\n){0,2}[0-9A-F]{2}){5,}");

    new Handler(
      *this, HEX, DefaultEncodings, DefaultOptions, &Scanner::hitHandler
    );
  }

  void Scanner::initScan(const scanner_params& sp) {
    Recorder = sp.fs.get_name("hex");
  }

  // Don't re-analyze hex bufs smaller than this
  const unsigned int opt_min_hex_buf = 64;

  size_t base16_decode_skipping_invalid(uint8_t* dst_start, const uint8_t* src, const uint8_t* src_end) {
    uint8_t* dst = dst_start;
    uint16_t byte;
    uint8_t msn, lsn;

    while (src < src_end) {
      msn = *src++;
      lsn = *src++;
      byte = BASE16_MSN[msn] | BASE16_LSN[lsn];
      if (byte < 0x100) {
        *dst++ = static_cast<uint8_t>(byte);
      }
      else {
        // A "byte" value over FF means we've hit something invalid. The
        // pattern requires that hex digits come in pairs, so the first
        // character is invalid. Just advance one byte (== backing up one
        // byte now, since we've already gone ahead two bytes).
        --src;
      }
    }

    return dst - dst_start;
  }

  void Scanner::decode(const sbuf_t& osbuf, size_t pos, size_t len, const scanner_params& sp, const recursion_control_block& rcb) {
    sbuf_t sbuf(osbuf, pos, len);  // the substring we are working with

    managed_malloc<uint8_t> b(sbuf.pagesize/2);
    if (b.buf == 0) return;

    const size_t p = base16_decode_skipping_invalid(
      b.buf, sbuf.buf, sbuf.buf+sbuf.pagesize
    );

    // Alert on byte sequences of 48, 128 or 256 bits
    if (p == 48/8 || p == 128/8 || p == 256/8) {
      // it validates; write original with context
      Recorder->write_buf(osbuf, pos, len);
      return; // Small keys don't get recursively analyzed
    }

    if (p > opt_min_hex_buf) {
      // NB: we manually add BASE16 here when recursing, because
      // rcb.partName is LIGHTGREP here, which is not useful.
      sbuf_t nsbuf(osbuf.pos0 + pos + "BASE16", b.buf, p, p, false);
      (*rcb.callback)(scanner_params(sp, nsbuf)); // recurse
    }
  }

  Scanner TheScanner;
}

extern "C"
void scan_base16_lg(const class scanner_params &sp, const recursion_control_block &rcb) {
  scan_lg(base16::TheScanner, sp, rcb);
}

#endif // HAVE_LIBLIGHTGREP
