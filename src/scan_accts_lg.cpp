#include "config.h"

// if liblightgrep isn't present, compiles to nothing
#ifdef HAVE_LIBLIGHTGREP

#include <algorithm>
#include <string>

#include "be13_api/bulk_extractor_i.h"
#include "histogram.h"
#include "scan_ccns2.h"
#include "pattern_scanner.h"
#include "pattern_scanner_utils.h"

namespace accts {
  const char* const DefaultEncodingsCStrings[] = {"UTF-8", "UTF-16LE"};

  const vector<string> DefaultEncodings(
    DefaultEncodingsCStrings,
    DefaultEncodingsCStrings +
      sizeof(DefaultEncodingsCStrings)/sizeof(DefaultEncodingsCStrings[0])
  );

  const vector<string> OnlyUTF8Encoding(1, "UTF-8");

  const vector<string> OnlyUTF16LEEncoding(1, "UTF-16LE");

  const LG_KeyOptions DefaultOptions = { 0, 1 }; // patterns, case-insensitive

  //
  // helper functions
  //

  bool is_pdf_box(const sbuf_t& sbuf, size_t pos) {
    const char box[] = "Box";
    const size_t c0 = pos >= 10 ? pos - 10 : 10 - pos - 1;
    const uint8_t* i = search(sbuf.buf + c0, sbuf.buf + pos, box, box + strlen(box));
    return i != sbuf.buf + pos;
/*
    return i != sbuf.buf + pos && (
      (i + 2 < sbuf.buf + pos && *(i+1) == ' ' && *(i+2) == '[')
      || *(i+1) == '['
    );
*/
  }

  inline bool valid_char(char ch) {
    return isdigit(ch) || isspace(ch) || ch=='[' || ch==']' ||
           ch=='<' || ch=='Z' || ch=='.' || ch=='l' || ch=='j';
  }

  bool valid_phone_utf16le(const sbuf_t& sbuf, size_t pos, size_t len) {
    // We want invalid characters before and after (assuming there is a
    // before and after)
    bool invalid_before = false;
    bool invalid_after = false;

    if (pos > 16) {
      for (size_t i = pos-16; i < pos; ++i) {
        if (sbuf[i] != '\0' && !valid_char(sbuf[i])) {
          invalid_before = true;
          break;
        }
      }
    }
    else {
      invalid_before = true;
    }

    if (sbuf.bufsize < pos+len+16) {
      for (size_t i = pos+len; i < pos+len+16; ++i) {
        if (sbuf[i] != '\0' && !valid_char(sbuf[i])) {
          invalid_after = true;
          break;
        }
      }
    }
    else {
      invalid_after = true;
    }

    /*
     * 2013-05-28: if followed by ' #{1,5} ' then it's not a phone either!
     */
    if (pos+len+10 < sbuf.bufsize) {
      if (sbuf[pos+len] == ' ' && sbuf[pos+len+1] == '\0' &&
          isdigit(sbuf[pos+len+2]) && sbuf[pos+len+3] == '\0') {
        for (size_t i = pos+len+2; i+3 < sbuf.bufsize && i < pos+len+16; i += 2) {
          if (isdigit(sbuf[i]) && sbuf[i+1] == '\0' &&
              sbuf[i+2] == ' ' && sbuf[i+3] == '\0') {
            return false; // not valid
          }
        }
      }
    }

    /* If it is followed by a dash and a number, it's not a phone number */
    if (pos+len+4 < sbuf.bufsize) {
      if (sbuf[pos+len] == '-' && sbuf[pos+len+1] == '\0' &&
          isdigit(sbuf[pos+len+2] && sbuf[pos+len+3] == '\0')) {
        return false;
      }
    }

    return invalid_before && invalid_after;
  }

  //
  // subpatterns
  //

//  const string END("([^0-9e.]|(\\.[^0-9]))");
  const string END("([^\\z2E\\z30-\\z39\\z45\\z65]|(\\.[^\\z30-\\z39]))");
  const string BLOCK("[0-9]{4}");
  const string DELIM("[- ]");
  const string DB("(" + BLOCK + DELIM + ")");
  const string SDB("([45][0-9]{3}" + DELIM + ")");
  const string TDEL("[ /.-]");

  const string PHONETEXT_UTF8_CTX("[^\\z41-\\z5A\\z61-\\z7A]");
  const string PHONETEXT_UTF16LE_CTX("([^\\z41-\\z5A\\z61-\\z7A]\\z00|[^\\z00])");
  const string PHONETEXT_COMMON("(tel[.ephon]*|fax|facsimile|DSN|telex|TTD|mobile|cell):?");
  const string PHONETEXT_UTF8("(" + PHONETEXT_UTF8_CTX + PHONETEXT_COMMON + ")");
  const string PHONETEXT_UTF16LE("(" + PHONETEXT_UTF16LE_CTX + PHONETEXT_COMMON + ")");

  const string YEAR("(19[0-9][0-9]|20[01][0-9])");
  const string MONTH("(Jan(uary)?|Feb(ruary)?|Mar(ch)?|Apr(il)?|May|Jun(e)?|Jul(y)?|Aug(ust)?|Sep(tember)?|Oct(ober)?|Nov(ember)?|Dec(ember)?|0?[1-9]|1[0-2])");
  const string DAY("([0-2]?[0-9]|3[01])");

  const string SYEAR("([0-9][0-9])");
  const string SMONTH("([01][0-2])");

  const string DATEA("(" + YEAR + "-" + MONTH + "-" + DAY + ")");
  const string DATEB("(" + YEAR + "/" + MONTH + "/" + DAY + ")");
  const string DATEC("(" + DAY + " " + MONTH + " " + YEAR + ")");
  const string DATED("(" + MONTH + " " + DAY + "[, ]+" + YEAR + ")");

  const string DATEFORMAT("(" + DATEA + "|" + DATEB + "|" + DATEC + "|" + DATED + ")");

  //
  // the scaner
  //

  class Scanner: public PatternScanner {
  public:
    Scanner(): PatternScanner("accts_lg"), CCN_Recorder(0), CCN_Track2_Recorder(0), Telephone_Recorder(0), Alert_Recorder(0), PII_Recorder(0), SIN_Recorder(0) {}
    virtual ~Scanner() {}

    virtual Scanner* clone() const { return new Scanner(*this); }

    virtual void startup(const scanner_params& sp);
    virtual void init(const scanner_params& sp);
    virtual void initScan(const scanner_params&);

    feature_recorder* CCN_Recorder;
    feature_recorder* CCN_Track2_Recorder;
    feature_recorder* Telephone_Recorder;
    feature_recorder* Alert_Recorder;
    feature_recorder* PII_Recorder;
    feature_recorder* SIN_Recorder;

    void ccnHitHandler(const LG_SearchHit& hit, const scanner_params& sp, const recursion_control_block& rcb);

    void ccnUTF16LEHitHandler(const LG_SearchHit& hit, const scanner_params& sp, const recursion_control_block& rcb);

    void ccnTrack2HitHandler(const LG_SearchHit& hit, const scanner_params& sp, const recursion_control_block& rcb);

    void ccnTrack2UTF16LEHitHandler(const LG_SearchHit& hit, const scanner_params& sp, const recursion_control_block& rcb);

    void telephoneHitHandler(const LG_SearchHit& hit, const scanner_params& sp, const recursion_control_block& rcb);

    void telephoneUTF16LEHitHandler(const LG_SearchHit& hit, const scanner_params& sp, const recursion_control_block& rcb);

    void telephoneTrailingCtxHitHandler(const LG_SearchHit& hit, const scanner_params& sp, const recursion_control_block& rcb);

    void telephoneTrailingCtxUTF16LEHitHandler(const LG_SearchHit& hit, const scanner_params& sp, const recursion_control_block& rcb);

    void validatedTelephoneHitHandler(const LG_SearchHit& hit, const scanner_params& sp, const recursion_control_block& rcb);

    void validatedTelephoneUTF16LEHitHandler(const LG_SearchHit& hit, const scanner_params& sp, const recursion_control_block& rcb);

    void bitlockerHitHandler(const LG_SearchHit& hit, const scanner_params& sp, const recursion_control_block& rcb);

    void bitlockerUTF16LEHitHandler(const LG_SearchHit& hit, const scanner_params& sp, const recursion_control_block& rcb);

    void piiHitHandler(const LG_SearchHit& hit, const scanner_params& sp, const recursion_control_block& rcb);

    void piiUTF16LEHitHandler(const LG_SearchHit& hit, const scanner_params& sp, const recursion_control_block& rcb);

    void sinHitHandler(const LG_SearchHit& hit, const scanner_params& sp, const recursion_control_block& rcb);

    void sinUTF16LEHitHandler(const LG_SearchHit& hit, const scanner_params& sp, const recursion_control_block& rcb);

    void sinHitHandler2(const LG_SearchHit& hit, const scanner_params& sp, const recursion_control_block& rcb);

    void sinUTF16LEHitHandler2(const LG_SearchHit& hit, const scanner_params& sp, const recursion_control_block& rcb);

    void dateHitHandler(const LG_SearchHit& hit, const scanner_params& sp, const recursion_control_block& rcb);

  private:
    Scanner(const Scanner& s):
      PatternScanner(s),
      CCN_Recorder(s.CCN_Recorder),
      CCN_Track2_Recorder(s.CCN_Track2_Recorder),
      Telephone_Recorder(s.Telephone_Recorder),
      Alert_Recorder(s.Alert_Recorder),
      PII_Recorder(s.PII_Recorder),
      SIN_Recorder(s.SIN_Recorder)
    {}

    Scanner& operator=(const Scanner&);
  };

  void Scanner::startup(const scanner_params& sp) {
    assert(sp.sp_version == scanner_params::CURRENT_SP_VERSION);
    assert(sp.info->si_version == scanner_info::CURRENT_SI_VERSION);

    sp.info->name            = "accts_lg";
    sp.info->author          = "Simson L. Garfinkel, modified by Tim Walsh";
    sp.info->description     = "scans for CCNs, track 2, PII (including SSN and Canadian SIN), and phone #s";
    sp.info->scanner_version = "1.0";

    // define the feature files this scanner creates
    sp.info->feature_names.insert("ccn");
    sp.info->feature_names.insert("pii");  // personally identifiable information
    sp.info->feature_names.insert("sin");  // canadian social insurance number
    sp.info->feature_names.insert("ccn_track2");
    sp.info->feature_names.insert("telephone");
    sp.info->histogram_defs.insert(histogram_def("ccn", "", "histogram"));
    sp.info->histogram_defs.insert(histogram_def("ccn_track2", "", "histogram"));

    // define the histograms to make
    sp.info->histogram_defs.insert(
      histogram_def("telephone", "", "histogram", HistogramMaker::FLAG_NUMERIC)
    );

    scan_ccns2_debug = sp.info->config->debug;           // get debug value
  }

  void Scanner::init(const scanner_params& sp) {
    //
    // patterns
    //

    // FIXME: leading context
    // FIXME: trailing context
    /* #### #### #### #### --- most credit card numbers*/
    const string REGEX2("[^\\z30-\\z39\\z41-\\z5A\\z61-\\z7A]" + SDB + DB + DB + BLOCK + END);

    new Handler(
      *this,
      REGEX2,
      OnlyUTF8Encoding,
      DefaultOptions,
      &Scanner::ccnHitHandler
    );

    const string REGEX2_UTF16LE("([^\\z30-\\z39\\z41-\\z5A\\z61-\\z7A]\\z00|[^\\z00])" + SDB + DB + DB + BLOCK + END);

    new Handler(
      *this,
      REGEX2_UTF16LE,
      OnlyUTF16LEEncoding,
      DefaultOptions,
      &Scanner::ccnUTF16LEHitHandler
    );

    // FIXME: leading context
    // FIXME: trailing context
    /* 3### ###### ######### --- 15 digits beginning with 3 and funny space. */
    /* Must be american express... */
    const string REGEX3("[^\\z30-\\z39\\z41-\\z5A\\z61-\\z7A\\z2E]3[0-9]{3}" + DELIM + "[0-9]{6}" + DELIM + "[0-9]{5}" + END);

    new Handler(
      *this,
      REGEX3,
      OnlyUTF8Encoding,
      DefaultOptions,
      &Scanner::ccnHitHandler
    );

    const string REGEX3_UTF16LE("([^\\z30-\\z39\\z41-\\z5A\\z61-\\z7A\\z2E]\\z00|[^\\z00])3[0-9]{3}" + DELIM + "[0-9]{6}" + DELIM + "[0-9]{5}" + END);

    new Handler(
      *this,
      REGEX3_UTF16LE,
      OnlyUTF16LEEncoding,
      DefaultOptions,
      &Scanner::ccnUTF16LEHitHandler
    );

    // FIXME: leading context
    // FIXME: trailing context
    /* 3### ###### ######### --- 15 digits beginning with 3 and funny space. */
    /* Must be american express... */
    const string REGEX4("[^\\z30-\\z39\\z41-\\z5A\\z61-\\z7A\\z2E]3[0-9]{14}" + END);

    new Handler(
      *this,
      REGEX4,
      OnlyUTF8Encoding,
      DefaultOptions,
      &Scanner::ccnHitHandler
    );

    const string REGEX4_UTF16LE("([^\\z30-\\z39\\z41-\\z5A\\z61-\\z7A\\z2E]\\z00|[^\\z00])3[0-9]{14}" + END);

    new Handler(
      *this,
      REGEX4_UTF16LE,
      OnlyUTF16LEEncoding,
      DefaultOptions,
      &Scanner::ccnUTF16LEHitHandler
    );

    // FIXME: leading context
    // FIXME: trailing context
    /* ###############  13-19 numbers as a block beginning with a 4 or 5
     * followed by something that is not a digit.
     * Yes, CCNs can now be up to 19 digits long.
     * http://www.creditcards.com/credit-card-news/credit-card-appearance-1268.php
     */
    const string REGEX5("[^\\z30-\\z39\\z41-\\z5A\\z61-\\z7A\\z2E][4-6][0-9]{15,18}" + END);

    new Handler(
      *this,
      REGEX5,
      OnlyUTF8Encoding,
      DefaultOptions,
      &Scanner::ccnHitHandler
    );

    const string REGEX5_UTF16LE("([^\\z30-\\z39\\z41-\\z5A\\z61-\\z7A\\z2E]\\z00|[^\\z00])[4-6][0-9]{15,18}" + END);

    new Handler(
      *this,
      REGEX5_UTF16LE,
      OnlyUTF16LEEncoding,
      DefaultOptions,
      &Scanner::ccnUTF16LEHitHandler
    );

    // FIXME: leading context
    /* ;###############=YYMM101#+? --- track2 credit card data */
    /* {SYEAR}{SMONTH} */
    /* ;CCN=05061010000000000738? */
    const string REGEX6("[^\\z30-\\z39\\z41-\\z5A\\z61-\\z7A][4-6][0-9]{15,18}=" + SYEAR + SMONTH + "101[0-9]{13}");

    new Handler(
      *this,
      REGEX6,
      OnlyUTF8Encoding,
      DefaultOptions,
      &Scanner::ccnTrack2HitHandler
    );

    const string REGEX6_UTF16LE("([^\\z30-\\z39\\z41-\\z5A\\z61-\\z7A]\\z00|[^\\z00])[4-6][0-9]{15,18}=" + SYEAR + SMONTH + "101[0-9]{13}");

    new Handler(
      *this,
      REGEX6_UTF16LE,
      OnlyUTF16LEEncoding,
      DefaultOptions,
      &Scanner::ccnTrack2UTF16LEHitHandler
    );

    // FIXME: trailing context
    // FIXME: leading context
    /* US phone numbers without area code in parens */
    /* New addition: If proceeded by " ####? ####? "
     * then do not consider this a phone number. We see a lot of that stuff in
     * PDF files.
     */
    const string REGEX7("[^\\z30-\\z39\\z41-\\z5A\\z61-\\z7A]([0-9]{3}" + TDEL + "){2}[0-9]{4}" + END);

    new Handler(
      *this,
      REGEX7,
      OnlyUTF8Encoding,
      DefaultOptions,
      &Scanner::validatedTelephoneHitHandler
    );

    const string REGEX7_UTF16LE("([^\\z30-\\z39\\z41-\\z5A\\z61-\\z7A]\\z00|[^\\z00])([0-9]{3}" + TDEL + "){2}[0-9]{4}" + END);

    new Handler(
      *this,
      REGEX7,
      OnlyUTF16LEEncoding,
      DefaultOptions,
      &Scanner::validatedTelephoneUTF16LEHitHandler
    );

    // FIXME: trailing context
    // FIXME: leading context
    /* US phone number with parens, like (215) 555-1212 */
    const string REGEX8("[^\\z30-\\z39\\z41-\\z5A\\z61-\\z7A]\\([0-9]{3}\\)" + TDEL + "?[0-9]{3}" + TDEL + "[0-9]{4}" + END);

    new Handler(
      *this,
      REGEX8,
      OnlyUTF8Encoding,
      DefaultOptions,
      &Scanner::telephoneTrailingCtxHitHandler
    );

    const string REGEX8_UTF16LE("([^\\z30-\\z39\\z41-\\z5A\\z61-\\z7A]\\z00|[^\\z00])\\([0-9]{3}\\)" + TDEL + "?[0-9]{3}" + TDEL + "[0-9]{4}" + END);

    new Handler(
      *this,
      REGEX8_UTF16LE,
      OnlyUTF16LEEncoding,
      DefaultOptions,
      &Scanner::telephoneTrailingCtxUTF16LEHitHandler
    );

    // FIXME: trailing context
    // FIXME: leading context
    /* Generalized international phone numbers */
    const string REGEX9("[^\\z30-\\z39\\z41-\\z5A\\z61-\\z7A]\\+[0-9]{1,3}(" + TDEL + "[0-9]{2,3}){2,6}[0-9]{2,4}" + END);

    new Handler(
      *this,
      REGEX9,
      OnlyUTF8Encoding,
      DefaultOptions,
      &Scanner::validatedTelephoneHitHandler
    );

    const string REGEX9_UTF16LE("([^\\z30-\\z39\\z41-\\z5A\\z61-\\z7A]\\z00|[^\\z00])\\+[0-9]{1,3}(" + TDEL + "[0-9]{2,3}){2,6}[0-9]{2,4}" + END);

    new Handler(
      *this,
      REGEX9,
      OnlyUTF16LEEncoding,
      DefaultOptions,
      &Scanner::validatedTelephoneHitHandler
    );

    // FIXME: leading context
    /* Generalized number with prefix */
    const string REGEX10(PHONETEXT_UTF8 + "[0-9/ .+]{7,18}");

    new Handler(
      *this,
      REGEX10,
      OnlyUTF8Encoding,
      DefaultOptions,
      &Scanner::telephoneHitHandler
    );

    const string REGEX10_UTF16LE(PHONETEXT_UTF16LE + "[0-9/ .+]{7,18}");

    new Handler(
      *this,
      REGEX10_UTF16LE,
      OnlyUTF16LEEncoding,
      DefaultOptions,
      &Scanner::telephoneUTF16LEHitHandler
    );

    // FIXME: leading context
    /* Generalized number with city code and prefix */
    const string REGEX11(PHONETEXT_UTF8 + "[0-9 +]+ ?\\([0-9]{2,4}\\) ?[\\-0-9]{4,8}");

    new Handler(
      *this,
      REGEX11,
      OnlyUTF8Encoding,
      DefaultOptions,
      &Scanner::telephoneHitHandler
    );

   const string REGEX11_UTF16LE(PHONETEXT_UTF16LE + "[0-9 +]+ ?\\([0-9]{2,4}\\) ?[\\-0-9]{4,8}");

    new Handler(
      *this,
      REGEX11_UTF16LE,
      OnlyUTF16LEEncoding,
      DefaultOptions,
      &Scanner::telephoneUTF16LEHitHandler
    );

    // FIXME: trailing context
    /* Generalized international phone numbers */
    const string REGEX12("fedex[^a-z]+([0-9]{4}[- ]?){2}[0-9]" + END);

    new Handler(
      *this,
      REGEX12,
      OnlyUTF8Encoding,
      DefaultOptions,
      &Scanner::piiHitHandler
    );

    new Handler(
      *this,
      REGEX12,
      OnlyUTF16LEEncoding,
      DefaultOptions,
      &Scanner::piiUTF16LEHitHandler
    );

    // FIXME: trailing context
    const string REGEX13("ssn:?[ \\t]+[0-9]{3}-?[0-9]{2}-?[0-9]{4}" + END);

    new Handler(
      *this,
      REGEX13,
      OnlyUTF8Encoding,
      DefaultOptions,
      &Scanner::piiHitHandler
    );

    new Handler(
      *this,
      REGEX13,
      OnlyUTF16LEEncoding,
      DefaultOptions,
      &Scanner::piiUTF16LEHitHandler
    );

    const string REGEX14("dob:?[ \\t]+" + DATEFORMAT);

    new Handler(
      *this,
      REGEX14,
      DefaultEncodings,
      DefaultOptions,
      &Scanner::dateHitHandler
    );

    // FIXME: trailing context
    const string REGEX15("sin:?[ \\t]+[0-9]{3}[ -]?[0-9]{3}[ -]?[0-9]{3}" + END);

    new Handler(
      *this,
      REGEX15,
      OnlyUTF8Encoding,
      DefaultOptions,
      &Scanner::sinHitHandler
    );

    new Handler(
      *this,
      REGEX15,
      OnlyUTF16LEEncoding,
      DefaultOptions,
      &Scanner::sinUTF16LEHitHandler
    );

    const string REGEX16("[^0-9][0-9]{3}-[0-9]{3}-[0-9]{3}" + END);

    new Handler(
      *this,
      REGEX16,
      OnlyUTF8Encoding,
      DefaultOptions,
      &Scanner::sinHitHandler2
    );

    new Handler(
      *this,
      REGEX16,
      OnlyUTF16LEEncoding,
      DefaultOptions,
      &Scanner::sinUTF16LEHitHandler2
    );

    // FIXME: leading context
    // FIXME: trailing context
    /* Possible BitLocker Recovery Key. */
    const string BITLOCKER("[^\\z30-\\z39]([0-9]{6}-){7}[0-9]{6}[^\\z30-\\z39]");

    new Handler(
      *this,
      BITLOCKER,
      OnlyUTF8Encoding,
      DefaultOptions,
      &Scanner::bitlockerHitHandler
    );

    const string BITLOCKER_UTF16LE("([^\\z30-\\z39]\\z00|[^\\z00])([0-9]{6}-){7}[0-9]{6}[^\\z30-\\z39]");

    new Handler(
      *this,
      BITLOCKER,
      OnlyUTF16LEEncoding,
      DefaultOptions,
      &Scanner::bitlockerUTF16LEHitHandler
    );
  }

  void Scanner::initScan(const scanner_params& sp) {
    CCN_Recorder = sp.fs.get_name("ccn");
    CCN_Track2_Recorder = sp.fs.get_name("ccn_track2");
    Telephone_Recorder = sp.fs.get_name("telephone");
    Alert_Recorder = sp.fs.get_alert_recorder();
    PII_Recorder = sp.fs.get_name("pii");
    SIN_Recorder = sp.fs.get_name("sin");
  }

  void Scanner::ccnHitHandler(const LG_SearchHit& hit, const scanner_params& sp, const recursion_control_block& rcb) {
    const size_t pos = hit.Start + 1;
    const size_t len = hit.End - (*(sp.sbuf.buf+hit.End-2) == '.' ? 2 : 1) - pos;

    if (valid_ccn(reinterpret_cast<const char*>(sp.sbuf.buf)+pos, len)) {
      CCN_Recorder->write_buf(sp.sbuf, pos, len);
    }
  }

  void Scanner::ccnUTF16LEHitHandler(const LG_SearchHit& hit, const scanner_params& sp, const recursion_control_block& rcb) {
    const size_t pos = hit.Start + (*(sp.sbuf.buf+hit.Start+1) == '\0' ? 2 : 1);    const size_t len = hit.End - pos;

    const string ascii(low_utf16le_to_ascii(sp.sbuf.buf+pos, len));
    if (valid_ccn(ascii.c_str(), ascii.size())) {
      CCN_Recorder->write_buf(sp.sbuf, pos, len);
    }
  }

  void Scanner::ccnTrack2HitHandler(const LG_SearchHit& hit, const scanner_params& sp, const recursion_control_block& rcb) {
    const size_t pos = hit.Start + 1;
    const size_t len = hit.End - pos;

    if (valid_ccn(reinterpret_cast<const char*>(sp.sbuf.buf)+pos, len)) {
      CCN_Recorder->write_buf(sp.sbuf, pos, len);
    }
  }

  void Scanner::ccnTrack2UTF16LEHitHandler(const LG_SearchHit& hit, const scanner_params& sp, const recursion_control_block& rcb) {
    const size_t pos = hit.Start + (*(sp.sbuf.buf+hit.Start+1) == '\0' ? 2 : 1);
    const size_t len = hit.End - pos;

    const string ascii(low_utf16le_to_ascii(sp.sbuf.buf+pos, len));
    if (valid_ccn(ascii.c_str(), ascii.size())) {
      CCN_Recorder->write_buf(sp.sbuf, pos, len);
    }
  }

  void Scanner::telephoneHitHandler(const LG_SearchHit& hit, const scanner_params& sp, const recursion_control_block& rcb) {
    Telephone_Recorder->write_buf(sp.sbuf, hit.Start+1, hit.End-hit.Start-1);
  }

  void Scanner::telephoneUTF16LEHitHandler(const LG_SearchHit& hit, const scanner_params& sp, const recursion_control_block& rcb) {
    const size_t start = hit.Start + (*(sp.sbuf.buf + hit.Start + 1) == '\0' ? 2 : 1);
    const size_t len = hit.End - start;

    Telephone_Recorder->write_buf(sp.sbuf, start, len);
  }

  void Scanner::telephoneTrailingCtxHitHandler(const LG_SearchHit& hit, const scanner_params& sp, const recursion_control_block& rcb) {
    Telephone_Recorder->write_buf(
      sp.sbuf,
      hit.Start+1,
      hit.End - (*(sp.sbuf.buf+hit.End-2) == '.' ? 2 : 1) - (hit.Start+1)
    );
  }

  void Scanner::telephoneTrailingCtxUTF16LEHitHandler(const LG_SearchHit& hit, const scanner_params& sp, const recursion_control_block& rcb) {
    Telephone_Recorder->write_buf(
      sp.sbuf,
      hit.Start+1,
      hit.End - (*(sp.sbuf.buf+hit.End-3) == '.' ? 3 : 1) -(hit.Start+1)
    );
  }

  void Scanner::validatedTelephoneHitHandler(const LG_SearchHit& hit, const scanner_params& sp, const recursion_control_block& rcb) {
    const size_t pos = hit.Start + 1;
    const size_t len = hit.End - (*(sp.sbuf.buf+hit.End-2) == '.' ? 2 : 1) - pos;
    if (valid_phone(sp.sbuf, pos, len)){
      if (!is_pdf_box(sp.sbuf, pos)) {
        Telephone_Recorder->write_buf(sp.sbuf, pos, len);
      }
    }
  }

  void Scanner::validatedTelephoneUTF16LEHitHandler(const LG_SearchHit& hit, const scanner_params& sp, const recursion_control_block& rcb) {
    const size_t pos = hit.Start + 1;
    const size_t len = hit.End - (*(sp.sbuf.buf+hit.End-2) == '.' ? 2 : 1) - pos;
    if (valid_phone_utf16le(sp.sbuf, pos, len)){
      Telephone_Recorder->write_buf(sp.sbuf, pos, len);
    }
  }

  void Scanner::bitlockerHitHandler(const LG_SearchHit& hit, const scanner_params& sp, const recursion_control_block& rcb) {
    Alert_Recorder->write(sp.sbuf.pos0 + hit.Start + 1, reinterpret_cast<const char*>(sp.sbuf.buf) + 1, "Possible BitLocker Recovery Key (ASCII).");
  }

  void Scanner::bitlockerUTF16LEHitHandler(const LG_SearchHit& hit, const scanner_params& sp, const recursion_control_block& rcb) {
    const size_t pos = hit.Start + (*(sp.sbuf.buf + hit.Start + 1) == '\0' ? 2 : 1);
    const size_t len = (hit.End - 1) - pos;

    Alert_Recorder->write(sp.sbuf.pos0 + pos, low_utf16le_to_ascii(sp.sbuf.buf + pos, len), "Possible BitLocker Recovery Key (UTF-16).");
  }

  void Scanner::piiHitHandler(const LG_SearchHit& hit, const scanner_params& sp, const recursion_control_block& rcb) {
    PII_Recorder->write_buf(
      sp.sbuf, hit.Start,
      hit.End - (*(sp.sbuf.buf+hit.End-2) == '.' ? 2 : 1) - hit.Start
    );
  }

  void Scanner::piiUTF16LEHitHandler(const LG_SearchHit& hit, const scanner_params& sp, const recursion_control_block& rcb) {
    PII_Recorder->write_buf(
      sp.sbuf, hit.Start,
      hit.End - (*(sp.sbuf.buf+hit.End-3) == '.' ? 3 : 1) - hit.Start
    );
  }

  void Scanner::sinHitHandler(const LG_SearchHit& hit, const scanner_params& sp, const recursion_control_block& rcb) {
    SIN_Recorder->write_buf(
      sp.sbuf, hit.Start,
      hit.End - (*(sp.sbuf.buf+hit.End-2) == '.' ? 2 : 1) - hit.Start
    );
  }

  void Scanner::sinUTF16LEHitHandler(const LG_SearchHit& hit, const scanner_params& sp, const recursion_control_block& rcb) {
    SIN_Recorder->write_buf(
      sp.sbuf, hit.Start,
      hit.End - (*(sp.sbuf.buf+hit.End-3) == '.' ? 3 : 1) - hit.Start
    );
  }

  void Scanner::sinHitHandler2(const LG_SearchHit& hit, const scanner_params& sp, const recursion_control_block& rcb) {
    SIN_Recorder->write_buf(
      sp.sbuf, hit.Start+1,
      hit.End - (*(sp.sbuf.buf+hit.End-2) == '.' ? 2 : 1) - hit.Start
    );
  }

  void Scanner::sinUTF16LEHitHandler2(const LG_SearchHit& hit, const scanner_params& sp, const recursion_control_block& rcb) {
    SIN_Recorder->write_buf(
      sp.sbuf, hit.Start+1,
      hit.End - (*(sp.sbuf.buf+hit.End-3) == '.' ? 3 : 1) - hit.Start
    );
  }

  void Scanner::dateHitHandler(const LG_SearchHit& hit, const scanner_params& sp, const recursion_control_block& rcb) {
    PII_Recorder->write_buf(sp.sbuf, hit.Start, hit.End - hit.Start);
  }

  Scanner TheScanner;
}

extern "C"
void scan_accts_lg(const class scanner_params &sp, const recursion_control_block &rcb) {
  scan_lg(accts::TheScanner, sp, rcb);
}

#endif // HAVE_LIBLIGHTGREP
