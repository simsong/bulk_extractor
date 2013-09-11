#include "config.h"

// if liblightgrep isn't present, compiles to nothing
#ifdef HAVE_LIBLIGHTGREP

#include <string>

#include "be13_api/bulk_extractor_i.h"
#include "histogram.h"
#include "pattern_scanner.h"

namespace accts {
  //
  // helper functions
  //

/*
  const size_t min_phone_digits = 6;

  bool has_min_digits(const char *buf) {
    unsigned int digit_count = 0;
    for (const char *cc = buf; *cc; ++cc) {
      if (isdigit(*cc)) digit_count += 1;
    }
    return digit_count >= min_phone_digits;
  }

  string utf16to8(const wstring &s) {
    string utf8_line;
    try {
      utf8::utf16to8(s.begin(), s.end(), back_inserter(utf8_line));
    }
    catch(utf8::invalid_utf16) {
      // Exception thrown: bad UTF16 encoding
      utf8_line = "";
    }
    return utf8_line;
  }
*/

  //
  // the scaner
  //
  class Scanner: public PatternScanner {
  public:
    Scanner(): PatternScanner("accts_lg"), CCN_Recorder(0), CCN_Track2_Recorder(0), Telephone_Recorder(0), Alert_Recorder(0), PII_Recorder(0) {}
    virtual ~Scanner() {}

    virtual void init(const scanner_params& sp);
    virtual void cleanup(const scanner_params&) {}

    feature_recorder* CCN_Recorder;
    feature_recorder* CCN_Track2_Recorder;
    feature_recorder* Telephone_Recorder;
    feature_recorder* Alert_Recorder;
    feature_recorder* PII_Recorder;

    void ccnHitHandler(const LG_SearchHit& hit, const scanner_params& sp, const recursion_control_block& rcb);

    void ccnTrack2HitHandler(const LG_SearchHit& hit, const scanner_params& sp, const recursion_control_block& rcb);

    void telephoneHitHandler(const LG_SearchHit& hit, const scanner_params& sp, const recursion_control_block& rcb);

    void validatedTelephoneHitHandler(const LG_SearchHit& hit, const scanner_params& sp, const recursion_control_block& rcb);

    void bitlockerHitHandler(const LG_SearchHit& hit, const scanner_params& sp, const recursion_control_block& rcb);

    void piiHitHandler(const LG_SearchHit& hit, const scanner_params& sp, const recursion_control_block& rcb);

  private:
    Scanner(const Scanner&);
    Scanner& operator=(const Scanner&);
  };

  void Scanner::init(const scanner_params& sp) {
    assert(sp.sp_version == scanner_params::CURRENT_SP_VERSION);
    assert(sp.info->si_version == scanner_info::CURRENT_SI_VERSION);

    sp.info->name            = "accts";
    sp.info->author          = "Simson L. Garfinkel";
    sp.info->description     = "scans for CCNs, track 2, and phone #s";
    sp.info->scanner_version = "1.0";

    sp.info->feature_names.insert("ccn");
    sp.info->feature_names.insert("pii");  // personally identifiable information
    sp.info->feature_names.insert("ccn_track2");
    sp.info->feature_names.insert("telephone");
    sp.info->histogram_defs.insert(histogram_def("ccn", "", "histogram"));
    sp.info->histogram_defs.insert(histogram_def("ccn_track2", "", "histogram"));
    sp.info->histogram_defs.insert(histogram_def("telephone", "", "histogram", HistogramMaker::FLAG_NUMERIC));
//    scan_ccns2_debug = sp.info->config->debug;           // get debug value

    CCN_Recorder = sp.fs.get_name("ccn");
    CCN_Track2_Recorder = sp.fs.get_name("ccn_track2");
    Telephone_Recorder = sp.fs.get_name("telephone");
    Alert_Recorder = sp.fs.get_alert_recorder();
    PII_Recorder = sp.fs.get_name("pii");
  }

  void Scanner::ccnHitHandler(const LG_SearchHit& hit, const scanner_params& sp, const recursion_control_block& rcb) {
  }

  void Scanner::ccnTrack2HitHandler(const LG_SearchHit& hit, const scanner_params& sp, const recursion_control_block& rcb) {
  }

  void Scanner::telephoneHitHandler(const LG_SearchHit& hit, const scanner_params& sp, const recursion_control_block& rcb) {
  }

  void Scanner::validatedTelephoneHitHandler(const LG_SearchHit& hit, const scanner_params& sp, const recursion_control_block& rcb) {
  }

  void Scanner::bitlockerHitHandler(const LG_SearchHit& hit, const scanner_params& sp, const recursion_control_block& rcb) {
  }

  void Scanner::piiHitHandler(const LG_SearchHit& hit, const scanner_params& sp, const recursion_control_block& rcb) {
  }

  Scanner TheScanner;

  const char* const DefaultEncodingsCStrings[] = {"UTF-8", "UTF-16LE"};

  const vector<string> DefaultEncodings(
    &DefaultEncodingsCStrings[0], &DefaultEncodingsCStrings[2]
  );

  //
  // subpatterns
  //

  const std::string END("([^0-9e.]|(\\.[^0-9]))");
  const std::string BLOCK("[0-9]{4}");
  const std::string DELIM("[- ]");
  const std::string DB("(" + BLOCK + DELIM + ")");
  const std::string SDB("([45][0-9]{3}" + DELIM + ")");
  const std::string TDEL("[ /.-]");

  const std::string PHONETEXT("([^a-z](tel[.ephon]*|fax|facsimile|DSN|telex|TTD|mobile|cell)):?");

  const std::string YEAR("(19[0-9][0-9]|20[01][0-9])");
  const std::string MONTH("(Jan(uary)?|Feb(ruary)?|Mar(ch)?|Apr(il)?|May|Jun(e)?|Jul(y)?|Aug(ust)?|Sep(tember)?|Oct(ober)?|Nov(ember)?|Dec(ember)?|0?[1-9]|1[0-2])");
  const std::string DAY("([0-2]?[0-9]|3[01])");

  const std::string SYEAR("([0-9][0-9])");
  const std::string SMONTH("([01][0-2])");
 
  const std::string DATEA("(" + YEAR + "-" + MONTH + "-" + DAY + ")");
  const std::string DATEB("(" + YEAR + "/" + MONTH + "/" + DAY + ")");
  const std::string DATEC("(" + DAY + " " + MONTH + " " + YEAR + ")");
  const std::string DATED("(" + MONTH + " " + DAY + "[, ]+" + YEAR + ")");

  const std::string DATEFORMAT("(" + DATEA + "|" + DATEB + "|" + DATEC + "|" + DATED + ")");

  //
  // patterns
  //

  // FIXME: kill this one?
  /* #### #### #### #### #### #### is definately not a CCN. */
  const std::string REGEX1("[^0-9a-z]" + DB + "{5}");

  // FIXME: leading context
  // FIXME: trailing context
  /* #### #### #### #### --- most credit card numbers*/
  const std::string REGEX2("[^0-9a-z]" + SDB + DB + DB + BLOCK + END);

  Handler REGEX2_Handler(
    TheScanner,
    REGEX2, 
    DefaultEncodings,
    bind(&Scanner::ccnHitHandler, &TheScanner, _1, _2, _3)
  );

  // FIXME: leading context
  // FIXME: trailing context
  /* 3### ###### ######### --- 15 digits beginning with 3 and funny space. */
  /* Must be american express... */ 
  const std::string REGEX3("[^0-9a-z.]3[0-9]{3}" + DELIM + "[0-9]{6}" + DELIM + "[0-9]{5}" + END);

  Handler REGEX3_Handler(
    TheScanner,
    REGEX3, 
    DefaultEncodings,
    bind(&Scanner::ccnHitHandler, &TheScanner, _1, _2, _3)
  );

  // FIXME: leading context
  // FIXME: trailing context
  /* 3### ###### ######### --- 15 digits beginning with 3 and funny space. */
  /* Must be american express... */ 
  const std::string REGEX4("[^0-9a-z.]3[0-9]{14}" + END);

  Handler REGEX4_Handler(
    TheScanner,
    REGEX4,
    DefaultEncodings,
    bind(&Scanner::ccnHitHandler, &TheScanner, _1, _2, _3)
  );

  // FIXME: leading context
  // FIXME: trailing context
  /* ###############  13-19 numbers as a block beginning with a 4 or 5
   * followed by something that is not a digit.
   * Yes, CCNs can now be up to 19 digits long. 
   * http://www.creditcards.com/credit-card-news/credit-card-appearance-1268.php
   */  
  const std::string REGEX5("[^0-9a-z.][4-6][0-9]{15,18}" + END);

  Handler REGEX5_Handler(
    TheScanner,
    REGEX5,
    DefaultEncodings,
    bind(&Scanner::ccnHitHandler, &TheScanner, _1, _2, _3)
  );

  // FIXME: leading context
  /* ;###############=YYMM101#+? --- track2 credit card data */
  /* {SYEAR}{SMONTH} */
  /* ;CCN=05061010000000000738? */
  const std::string REGEX6("[^0-9a-z][4-6][0-9]{15,18}=" + SYEAR + SMONTH + "101[0-9]{13}");

  Handler REGEX6_Handler(
    TheScanner,
    REGEX6,
    DefaultEncodings,
    bind(&Scanner::ccnTrack2HitHandler, &TheScanner, _1, _2, _3)
  );

  // FIXME: trailing context
  // FIXME: leading context
  /* US phone numbers without area code in parens */
  /* New addition: If proceeded by " ####? ####? " 
   * then do not consider this a phone number. We see a lot of that stuff in
   * PDF files.
   */
  const std::string REGEX7("[^0-9a-z]([0-9]{3}" + TDEL + "){2}[0-9]{4}" + END);

  Handler REGEX7_Handler(
    TheScanner,
    REGEX7,
    DefaultEncodings,
    bind(&Scanner::validatedTelephoneHitHandler, &TheScanner, _1, _2, _3)
  );

  // FIXME: trailing context
  // FIXME: leading context
  /* US phone number with parens, like (215) 555-1212 */
  const std::string REGEX8("[^0-9a-z]\\([0-9]{3}\\)" + TDEL + "?[0-9]{3}" + TDEL + "[0-9]{4}" + END);

  Handler REGEX8_Handler(
    TheScanner,
    REGEX8,
    DefaultEncodings,
    bind(&Scanner::telephoneHitHandler, &TheScanner, _1, _2, _3)
  );

  // FIXME: trailing context
  // FIXME: leading context
  /* Generalized international phone numbers */
  const std::string REGEX9("[^0-9a-z]\\+[0-9]{1,3}(" + TDEL + "[0-9]{2,3}){2,6}[0-9]{2,4}" + END);

  Handler REGEX9_Handler(
    TheScanner,
    REGEX9,
    DefaultEncodings,
    bind(&Scanner::validatedTelephoneHitHandler, &TheScanner, _1, _2, _3)
  );
  
  /* Generalized number with prefix */
  const std::string REGEX10(PHONETEXT + "[0-9/ .+]{7,18}");

  Handler REGEX10_Handler(
    TheScanner,
    REGEX10,
    DefaultEncodings,
    bind(&Scanner::telephoneHitHandler, &TheScanner, _1, _2, _3)
  );

  /* Generalized number with city code and prefix */
  const std::string REGEX11(PHONETEXT + "[0-9 +]+ ?\\([0-9]{2,4}\\) ?[\\-0-9]{4,8}");

  Handler REGEX11_Handler(
    TheScanner,
    REGEX11,
    DefaultEncodings,
    bind(&Scanner::telephoneHitHandler, &TheScanner, _1, _2, _3)
  );

  // FIXME: trailing context
  /* Generalized international phone numbers */
  const std::string REGEX12("fedex[^a-z]+([0-9]{4}[- ]?){2}[0-9]" + END);

  Handler REGEX12_Handler(
    TheScanner,
    REGEX12,
    DefaultEncodings,
    bind(&Scanner::piiHitHandler, &TheScanner, _1, _2, _3)
  );

  // FIXME: trailing context
  const std::string REGEX13("ssn:?[ \\t]+[0-9]{3}-?[0-9]{2}-?[0-9]{4}" + END);

  Handler REGEX13_Handler(
    TheScanner,
    REGEX13,
    DefaultEncodings,
    bind(&Scanner::piiHitHandler, &TheScanner, _1, _2, _3)
  );

  const std::string REGEX14("dob:?[ \\t]+" + DATEFORMAT);

  Handler REGEX14_Handler(
    TheScanner,
    REGEX14,
    DefaultEncodings,
    bind(&Scanner::piiHitHandler, &TheScanner, _1, _2, _3)
  );

  // FIXME: trailing context
  /* Possible BitLocker Recovery Key. */
  const std::string BITLOCKER("[^\\z30-\\z39]([0-9]{6}-){7}[0-9]{6}[^\\z30-\\z39]");

  Handler BITLOCKER_Handler(
    TheScanner,
    BITLOCKER,
    DefaultEncodings,
    bind(&Scanner::bitlockerHitHandler, &TheScanner, _1, _2, _3)
  );

  /*
   * Common box arrays found in PDF files
   * With more testing this can and will still be tweaked
   */
  const std::string PDF_BOX("box ?\\[[0-9 -]{0,40}\\]");

  /*
   * Common rectangles found in PDF files
   *  With more testing this can and will still be tweaked
   */
  const std::string PDF_RECT("\\[ ?([0-9.-]{1,12} ){3}[0-9.-]{1,12} ?\\]");
}

extern "C"
void scan_accts_lg(const class scanner_params &sp, const recursion_control_block &rcb) {
  using namespace accts;

  if (sp.phase == scanner_params::PHASE_STARTUP) {
    cerr << "scan_accts_lg - init" << endl;
    TheScanner.init(sp);
    LightgrepController::Get().addScanner(TheScanner);
  }
  else if (sp.phase == scanner_params::PHASE_SHUTDOWN) {
    cerr << "scan_accts_lg - cleanup" << endl;
    TheScanner.cleanup(sp);
  }
}

#endif // HAVE_LIBLIGHTGREP
