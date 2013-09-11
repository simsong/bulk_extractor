#include "config.h"

// if liblightgrep isn't present, compiles to nothing
#ifdef HAVE_LIBLIGHTGREP

#include <string>

#include "be13_api/bulk_extractor_i.h"
#include "histogram.h"
#include "pattern_scanner.h"

namespace email {
  //
  // helper functions
  //

  // Address some common false positives in email scanner 
  inline bool validate_email(const char *email) {
    if (strstr(email, "..")) return false;
    return true;
  }

  /** return the offset of the domain in an email address.
   * returns 0 if the domain is not found.
   * the domain extends to the end of the email address
   */
  inline size_t find_domain_in_email(const unsigned char *buf, size_t buflen) {
// FIXME: replace loop with strchr?
    for (size_t i = 0; i < buflen; ++i) {
      if (buf[i] == '@') return i+1;
    }
    return 0;        // not found
  }

  inline size_t find_domain_in_url(const unsigned char *buf,size_t buflen, size_t *domain_len) {
    for (size_t i = 2; i < buflen-1; ++i) {
      if (buf[i-1] == '/' && buf[i-2] == '/') {
        for (size_t j = i; j < buflen; ++j) {
          if (buf[j] == '/' || buf[j] == ':') {
            *domain_len = (j-i);
            return i;
          }
        }
        // Looks like it's the rest of the buffer
        *domain_len = buflen-i;
        return i;
      }
    }
    return 0;        // not found
  }

  //
  // the scanner
  //
  
  class Scanner: public PatternScanner {
  public:
    Scanner(): PatternScanner("email_lg"), RFC822_Recorder(0), Email_Recorder(0), Domain_Recorder(0), Ether_Recorder(0) {}
    virtual ~Scanner() {}

    virtual void init(const scanner_params& sp);
    virtual void cleanup(const scanner_params&) {}

    feature_recorder* RFC822_Recorder;
    feature_recorder* Email_Recorder;
    feature_recorder* Domain_Recorder;
    feature_recorder* Ether_Recorder;

    void defaultHitHandler(feature_recorder* fr, const LG_SearchHit& hit, const scanner_params& sp, const recursion_control_block& rcb) {
      fr->write_buf(sp.sbuf, hit.Start, hit.End - hit.Start);
    }

    void emailHitHandler(const LG_SearchHit& hit, const scanner_params& sp, const recursion_control_block& rcb);

    void ipaddrHitHandler(const LG_SearchHit& hit, const scanner_params& sp, const recursion_control_block& rcb);

    void etherHitHandler(const LG_SearchHit& hit, const scanner_params& sp, const recursion_control_block& rcb);

    void protoHitHandler(const LG_SearchHit& hit, const scanner_params& sp, const recursion_control_block& rcb);

  private:
    Scanner(const Scanner&);
    Scanner& operator=(const Scanner&);
  };

  void Scanner::init(const scanner_params& sp) {
    assert(sp.sp_version == scanner_params::CURRENT_SP_VERSION);
    assert(sp.info->si_version == scanner_info::CURRENT_SI_VERSION);

    sp.info->name            = "email";
    sp.info->author          = "Simson L. Garfinkel";
    sp.info->description     = "Scans for email addresses, domains, URLs, RFC822 headers, etc.";
    sp.info->scanner_version = "1.0";

    // define the feature files this scanner created
    sp.info->feature_names.insert("email");
    sp.info->feature_names.insert("domain");
    sp.info->feature_names.insert("url");
    sp.info->feature_names.insert("rfc822");
    sp.info->feature_names.insert("ether");

    // define the histograms to make
    sp.info->histogram_defs.insert(histogram_def("email", "", "histogram", HistogramMaker::FLAG_LOWERCASE));
    sp.info->histogram_defs.insert(histogram_def("domain", "", "histogram"));
    sp.info->histogram_defs.insert(histogram_def("url", "", "histogram"));
    sp.info->histogram_defs.insert(histogram_def("url", "://([^/]+)", "services"));
    sp.info->histogram_defs.insert(histogram_def("url", "://((cid-[0-9a-f])+[a-z.].live.com/)", "microsoft-live"));
    sp.info->histogram_defs.insert(histogram_def("url", "://[-_a-z0-9.]+facebook.com/.*[&?]{1}id=([0-9]+)", "facebook-id"));
    sp.info->histogram_defs.insert(histogram_def("url", "://[-_a-z0-9.]+facebook.com/([a-zA-Z0-9.]*[^/?&]$)", "facebook-address",  HistogramMaker::FLAG_LOWERCASE));
    sp.info->histogram_defs.insert(histogram_def("url", "search.*[?&/;fF][pq]=([^&/]+)", "searches"));

    RFC822_Recorder = sp.fs.get_name("rfc822");
    Email_Recorder = sp.fs.get_name("email");
    Domain_Recorder = sp.fs.get_name("domain");
    Ether_Recorder = sp.fs.get_name("ether");
  }

  void Scanner::emailHitHandler(const LG_SearchHit& hit, const scanner_params& sp, const recursion_control_block& rcb) {
    const size_t len = hit.End - hit.Start;

/*
    if (validate_email(sp.sbuf.buf)) {
      Email_Recorder->write_buf(sp.sbuf, hit.Start, len);
	    const size_t domain_start = find_domain_in_email(sp.sbuf.buf + hit.Start, len);
      if (domain_start > 0) {
        Domain_Recorder->write_buf(sp.sbuf, hit.Start + domain_start, len - domain_start);
      }
    }
*/
  }

  void Scanner::ipaddrHitHandler(const LG_SearchHit& hit, const scanner_params& sp, const recursion_control_block& rcb) {
  }

  void Scanner::etherHitHandler(const LG_SearchHit& hit, const scanner_params& sp, const recursion_control_block& rcb) {
  }

  void Scanner::protoHitHandler(const LG_SearchHit& hit, const scanner_params& sp, const recursion_control_block& rcb) {
  }

  Scanner TheScanner;

  const char* const DefaultEncodingsCStrings[] = {"UTF-8", "UTF-16LE"};

  const vector<string> DefaultEncodings(
    &DefaultEncodingsCStrings[0], &DefaultEncodingsCStrings[2]
  );

  //
  // subpatterns
  //

  const std::string INUM("(1?[0-9]{1,2}|2([0-4][0-9]|5[0-5]))");
  const std::string HEX("[0-9a-f]");
  const std::string ALNUM("[a-zA-Z0-9]");

  const std::string PC("[\\x20-\\x7E]");

  const std::string TLD("(AC|AD|AE|AERO|AF|AG|AI|AL|AM|AN|AO|AQ|AR|ARPA|AS|ASIA|AT|AU|AW|AX|AZ|BA|BB|BD|BE|BF|BG|BH|BI|BIZ|BJ|BL|BM|BN|BO|BR|BS|BT|BV|BW|BY|BZ|CA|CAT|CC|CD|CF|CG|CH|CI|CK|CL|CM|CN|CO|COM|COOP|CR|CU|CV|CX|CY|CZ|DE|DJ|DK|DM|DO|DZ|EC|EDU|EE|EG|EH|ER|ES|ET|EU|FI|FJ|FK|FM|FO|FR|GA|GB|GD|GE|GF|GG|GH|GI|GL|GM|GN|GOV|GP|GQ|GR|GS|GT|GU|GW|GY|HK|HM|HN|HR|HT|HU|ID|IE|IL|IM|IN|INFO|INT|IO|IQ|IR|IS|IT|JE|JM|JO|JOBS|JP|KE|KG|KH|KI|KM|KN|KP|KR|KW|KY|KZ|LA|LB|LC|LI|LK|LR|LS|LT|LU|LV|LY|MA|MC|MD|ME|MF|MG|MH|MIL|MK|ML|MM|MN|MO|MOBI|MP|MQ|MR|MS|MT|MU|MUSEUM|MV|MW|MX|MY|MZ|NA|NAME|NC|NE|NET|NF|NG|NI|NL|NO|NP|NR|NU|NZ|OM|ORG|PA|PE|PF|PG|PH|PK|PL|PM|PN|PR|PRO|PS|PT|PW|PY|QA|RE|RO|RS|RU|RW|SA|SB|SC|SD|SE|SG|SH|SI|SJ|SK|SL|SM|SN|SO|SR|ST|SU|SV|SY|SZ|TC|TD|TEL|TF|TG|TH|TJ|TK|TL|TM|TN|TO|TP|TR|TRAVEL|TT|TV|TW|TZ|UA|UG|UK|UM|US|UY|UZ|VA|VC|VE|VG|VI|VN|VU|WF|WS|YE|YT|YU|ZA|ZM|ZW)");

  const std::string YEAR("(19[6-9][0-9]|20[0-1][0-9])");
  const std::string DAYOFWEEK("(Mon|Tue|Wed|Thu|Fri|Sat|Sun)");
  const std::string MONTH("(Jan|Feb|Mar|Apr|May|Jun|Jul|Aug|Sep|Oct|Nov|Dec)");
  const std::string ABBREV("(UT|GMT|EST|EDT|CST|CDT|MST|MDT|PST|PDT|[ZAMNY])");

  //
  // patterns
  //

  const std::string DATE(DAYOFWEEK + ",[ \\t\\n]+[0-9]{1,2}[ \\t\\n]+" + MONTH + "[ \\t\\n]+" + YEAR + "[ \\t\\n]+[0-2][0-9]:[0-5][0-9]:[0-5][0-9][ \\t\\n]+([+-][0-2][0-9][0314][05]|" + ABBREV + ")");

  Handler DATE_Handler(
    TheScanner,
    DATE, 
    DefaultEncodings,
    bind(&Scanner::defaultHitHandler, &TheScanner, TheScanner.RFC822_Recorder, _1, _2, _3)
  );

  const std::string MESSAGE_ID("Message-ID:[ \\t\\n]?<" + PC + "{1,80}>");

  Handler MESSAGE_ID_Handler(
    TheScanner,
    MESSAGE_ID, 
    DefaultEncodings,
    bind(&Scanner::defaultHitHandler, &TheScanner, TheScanner.RFC822_Recorder, _1, _2, _3)
  );

  const std::string SUBJECT("Subject:[ \\t]?" + PC + "{1,80}");

  Handler SUBJECT_Handler(
    TheScanner,
    SUBJECT, 
    DefaultEncodings,
    bind(&Scanner::defaultHitHandler, &TheScanner, TheScanner.RFC822_Recorder, _1, _2, _3)
  );

  const std::string COOKIE("Cookie:[ \\t]?" + PC + "{1,80}");

  Handler COOKIE_Handler(
    TheScanner,
    COOKIE, 
    DefaultEncodings,
    bind(&Scanner::defaultHitHandler, &TheScanner, TheScanner.RFC822_Recorder, _1, _2, _3)
  );

  const std::string HOST("Host:[ \\t]?[a-zA-Z0-9._]{1,64}");

  Handler HOST_Handler(
    TheScanner,
    HOST, 
    DefaultEncodings,
    bind(&Scanner::defaultHitHandler, &TheScanner, TheScanner.RFC822_Recorder, _1, _2, _3)
  );

  // FIXME: trailing context
  const std::string EMAIL(ALNUM + "([a-zA-Z0-9._%\\-+]*?" + ALNUM + ")?@(" + ALNUM + "([a-zA-Z0-9\\-]*?" + ALNUM + ")?\\.)+" + TLD + "([^a-zA-Z]|[\\z00-\\zFF][^\\z00])");

   Handler EMAIL_Handler(
    TheScanner,
    EMAIL, 
    DefaultEncodings,
    bind(&Scanner::emailHitHandler, &TheScanner, _1, _2, _3)
  );

  // FIXME: leading context
  // FIXME: trailing context
  /* Numeric IP addresses. Get the context before and throw away some things */
  const std::string IP("[^0-9.]" + INUM + "(\\." + INUM + "){3}[^0-9\\-.+A-Z_]");

  Handler IP_Handler(
    TheScanner,
    IP, 
    DefaultEncodings,
    bind(&Scanner::ipaddrHitHandler, &TheScanner, _1, _2, _3)
  );

  // FIXME: leading context
  // FIXME: trailing context
  // FIXME: should we be searching for all uppercase MAC addresses as well?
  /* found a possible MAC address! */
  const std::string MAC("[^0-9A-Z:]" + HEX + "{2}(:" + HEX + "{2}){5}[^0-9A-Z:]");

  Handler MAC_Handler(
    TheScanner,
    MAC, 
    DefaultEncodings,
    bind(&Scanner::etherHitHandler, &TheScanner, _1, _2, _3)
  );

  // for reasons that aren't clear, there are a lot of net protocols that have
  // an http://domain in them followed by numbers. So this counts the number of
  // slashes and if it is only 2 the size is pruned until the last character
  // is a letter
  // FIXME: trailing context
  const std::string PROTO("(https?|afp|smb)://[a-zA-Z0-9_%/\\-+@:=&?#~.;]+([^a-zA-Z0-9_%/\\-+@:=&?#~.;]|[\\z00-\\zFF][^\\z00])");

  Handler PROTO_Handler(
    TheScanner,
    PROTO, 
    DefaultEncodings,
    bind(&Scanner::protoHitHandler, &TheScanner, _1, _2, _3)
  );
}

extern "C"
void scan_email_lg(const class scanner_params &sp, const recursion_control_block &rcb) {
  using namespace email;

  if (sp.phase == scanner_params::PHASE_STARTUP) {
    cerr << "scan_email_lg - init" << endl;
    TheScanner.init(sp);
    LightgrepController::Get().addScanner(TheScanner);
  }
  else if (sp.phase == scanner_params::PHASE_SHUTDOWN) {
    cerr << "scan_email_lg - cleanup" << endl;
    TheScanner.cleanup(sp);
  }
}

#endif // HAVE_LIBLIGHTGREP

