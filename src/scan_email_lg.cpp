#include "config.h"

// if liblightgrep isn't present, compiles to nothing
#ifdef HAVE_LIBLIGHTGREP

#include <algorithm>
#include <functional>
#include <string>

#include "be13_api/bulk_extractor_i.h"
#include "histogram.h"
#include "pattern_scanner.h"
#include "utils.h"

using namespace std;

namespace email {
  const char* const DefaultEncodingsCStrings[] = {"UTF-8", "UTF-16LE"};

  const vector<string> DefaultEncodings(
    DefaultEncodingsCStrings,
    DefaultEncodingsCStrings +
      sizeof(DefaultEncodingsCStrings)/sizeof(DefaultEncodingsCStrings[0])
  );

  const LG_KeyOptions DefaultOptions = { 0, 1 }; // patterns, case-insensitive

  //
  // subpatterns
  //

  const string INUM("(1?[0-9]{1,2}|2([0-4][0-9]|5[0-5]))");
  const string HEX("[0-9a-f]");
  const string ALNUM("[a-zA-Z0-9]");

  const string PC("[\\x20-\\x7E]");

  const string TLD("(AC|AD|AE|AERO|AF|AG|AI|AL|AM|AN|AO|AQ|AR|ARPA|AS|ASIA|AT|AU|AW|AX|AZ|BA|BB|BD|BE|BF|BG|BH|BI|BIZ|BJ|BL|BM|BN|BO|BR|BS|BT|BV|BW|BY|BZ|CA|CAT|CC|CD|CF|CG|CH|CI|CK|CL|CM|CN|CO|COM|COOP|CR|CU|CV|CX|CY|CZ|DE|DJ|DK|DM|DO|DZ|EC|EDU|EE|EG|EH|ER|ES|ET|EU|FI|FJ|FK|FM|FO|FR|GA|GB|GD|GE|GF|GG|GH|GI|GL|GM|GN|GOV|GP|GQ|GR|GS|GT|GU|GW|GY|HK|HM|HN|HR|HT|HU|ID|IE|IL|IM|IN|INFO|INT|IO|IQ|IR|IS|IT|JE|JM|JO|JOBS|JP|KE|KG|KH|KI|KM|KN|KP|KR|KW|KY|KZ|LA|LB|LC|LI|LK|LR|LS|LT|LU|LV|LY|MA|MC|MD|ME|MF|MG|MH|MIL|MK|ML|MM|MN|MO|MOBI|MP|MQ|MR|MS|MT|MU|MUSEUM|MV|MW|MX|MY|MZ|NA|NAME|NC|NE|NET|NF|NG|NI|NL|NO|NP|NR|NU|NZ|OM|ORG|PA|PE|PF|PG|PH|PK|PL|PM|PN|PR|PRO|PS|PT|PW|PY|QA|RE|RO|RS|RU|RW|SA|SB|SC|SD|SE|SG|SH|SI|SJ|SK|SL|SM|SN|SO|SR|ST|SU|SV|SY|SZ|TC|TD|TEL|TF|TG|TH|TJ|TK|TL|TM|TN|TO|TP|TR|TRAVEL|TT|TV|TW|TZ|UA|UG|UK|UM|US|UY|UZ|VA|VC|VE|VG|VI|VN|VU|WF|WS|YE|YT|YU|ZA|ZM|ZW)");

  const string YEAR("(19[6-9][0-9]|20[0-1][0-9])");
  const string DAYOFWEEK("(Mon|Tue|Wed|Thu|Fri|Sat|Sun)");
  const string MONTH("(Jan|Feb|Mar|Apr|May|Jun|Jul|Aug|Sep|Oct|Nov|Dec)");
  const string ABBREV("(UT|GMT|EST|EDT|CST|CDT|MST|MDT|PST|PDT|[ZAMNY])");

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
  inline size_t find_domain_in_email(const char *buf, size_t buflen) {
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

  bool valid_ether_addr(const sbuf_t& sbuf, size_t pos) {
    if (sbuf.memcmp((const uint8_t *)"00:00:00:00:00:00", pos, 17) == 0) {
      return false;
    }

    if (sbuf.memcmp((const uint8_t *)"00:11:22:33:44:55", pos, 17) == 0) {
      return false;
    }

    /* Perform a quick histogram analysis.
     * For each group of characters, create a value based on the two digits.
     * There is no need to convert them to their 'actual' value.
     * Don't accept a histogram that has 3 values. That could be
     * 11:11:11:11:22:33
     * Require 4, 5 or 6.
     * If we have 3 or more distinct values, then treat it good.
     * Otherwise its is some pattern we don't want.
     */
    set<uint16_t> ctr;
    for (uint32_t i = 0; i < 6; ++i) {  // loop for each group of numbers
      u_char ch1 = sbuf[pos+i*3];
      u_char ch2 = sbuf[pos+i*3+1];
      uint16_t val = (ch1 << 8) + (ch2); // create a value of the two characters (it's not
      ctr.insert(val);
    }

    return ctr.size() >= 4;
  }

  //
  // the scanner
  //

  class Scanner: public PatternScanner {
  public:
    Scanner(): PatternScanner("email_lg"), RFC822_Recorder(0), Email_Recorder(0), Domain_Recorder(0), Ether_Recorder(0), URL_Recorder(0) {}
    virtual ~Scanner() {}

    virtual Scanner* clone() const { return new Scanner(*this); }

    virtual void startup(const scanner_params& sp);
    virtual void init(const scanner_params& sp);
    virtual void initScan(const scanner_params& sp);

    feature_recorder* RFC822_Recorder;
    feature_recorder* Email_Recorder;
    feature_recorder* Domain_Recorder;
    feature_recorder* Ether_Recorder;
    feature_recorder* URL_Recorder;

    void rfc822HitHandler(const LG_SearchHit& hit, const scanner_params& sp, const recursion_control_block& rcb);

    void emailHitHandler(const LG_SearchHit& hit, const scanner_params& sp, const recursion_control_block& rcb);

    void ipaddrHitHandler(const LG_SearchHit& hit, const scanner_params& sp, const recursion_control_block& rcb);

    void etherHitHandler(const LG_SearchHit& hit, const scanner_params& sp, const recursion_control_block& rcb);

    void protoHitHandler(const LG_SearchHit& hit, const scanner_params& sp, const recursion_control_block& rcb);

  private:
    Scanner(const Scanner& s):
      PatternScanner(s),
      RFC822_Recorder(s.RFC822_Recorder),
      Email_Recorder(s.Email_Recorder),
      Domain_Recorder(s.Domain_Recorder),
      Ether_Recorder(s.Ether_Recorder),
      URL_Recorder(s.URL_Recorder)
    {}

    Scanner& operator=(const Scanner&);
  };

  void Scanner::startup(const scanner_params& sp) {
    assert(sp.sp_version == scanner_params::CURRENT_SP_VERSION);
    assert(sp.info->si_version == scanner_info::CURRENT_SI_VERSION);

    sp.info->name            = "email_lg";
    sp.info->author          = "Simson L. Garfinkel";
    sp.info->description     = "Scans for email addresses, domains, URLs, RFC822 headers, etc.";
    sp.info->scanner_version = "1.0";

    // define the feature files this scanner creates
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
  }

  void Scanner::init(const scanner_params& sp) {
    //
    // patterns
    //

    const string DATE(DAYOFWEEK + ",[ \\t\\n]+[0-9]{1,2}[ \\t\\n]+" + MONTH + "[ \\t\\n]+" + YEAR + "[ \\t\\n]+[0-2][0-9]:[0-5][0-9]:[0-5][0-9][ \\t\\n]+([+-][0-2][0-9][0314][05]|" + ABBREV + ")");

    new Handler(
      *this,
      DATE,
      DefaultEncodings,
      DefaultOptions,
      &Scanner::rfc822HitHandler
    );

    const string MESSAGE_ID("Message-ID:[ \\t\\n]?<" + PC + "{1,80}>");

    new Handler(
      *this,
      MESSAGE_ID,
      DefaultEncodings,
      DefaultOptions,
      &Scanner::rfc822HitHandler
    );

    const string SUBJECT("Subject:[ \\t]?" + PC + "{1,80}");

    new Handler(
      *this,
      SUBJECT,
      DefaultEncodings,
      DefaultOptions,
      &Scanner::rfc822HitHandler
    );

    const string COOKIE("Cookie:[ \\t]?" + PC + "{1,80}");

    new Handler(
      *this,
      COOKIE,
      DefaultEncodings,
      DefaultOptions,
      &Scanner::rfc822HitHandler
    );

    const string HOST("Host:[ \\t]?[a-zA-Z0-9._]{1,64}");

    new Handler(
      *this,
      HOST,
      DefaultEncodings,
      DefaultOptions,
      &Scanner::rfc822HitHandler
    );

    // FIXME: trailing context
    const string EMAIL(ALNUM + "([a-zA-Z0-9._%\\-+]*?" + ALNUM + ")?@(" + ALNUM + "([a-zA-Z0-9\\-]*?" + ALNUM + ")?\\.)+" + TLD + "([^a-zA-Z]|[\\z00-\\zFF][^\\z00])");
//    const string EMAIL(ALNUM + "([a-zA-Z0-9._%\\-+]*?" + ALNUM + ")?@(" + ALNUM + "([a-zA-Z0-9\\-]*?" + ALNUM + ")?\\.)+" + TLD + "[^a-zA-Z]");

    new Handler(
      *this,
      EMAIL,
      DefaultEncodings,
      DefaultOptions,
      &Scanner::emailHitHandler
    );

    // FIXME: leading context
    // FIXME: trailing context
    // Numeric IP addresses. Get the context before and throw away some things
    const string IP("[^0-9.]" + INUM + "(\\." + INUM + "){3}[^0-9\\-.+A-Z_]");

    new Handler(
      *this,
      IP,
      DefaultEncodings,
      DefaultOptions,
      &Scanner::ipaddrHitHandler
    );

    // FIXME: leading context
    // FIXME: trailing context
    // FIXME: should we be searching for all uppercase MAC addresses as well?
    // found a possible MAC address!
    const string MAC("[^0-9A-Z:]" + HEX + "{2}(:" + HEX + "{2}){5}[^0-9A-Z:]");

    new Handler(
      *this,
      MAC,
      DefaultEncodings,
      DefaultOptions,
      &Scanner::etherHitHandler
    );

    // for reasons that aren't clear, there are a lot of net protocols that have
    // an http://domain in them followed by numbers. So this counts the number of
    // slashes and if it is only 2 the size is pruned until the last character
    // is a letter
    // FIXME: trailing context
    const string PROTO("(https?|afp|smb)://[a-zA-Z0-9_%/\\-+@:=&?#~.;]+([^a-zA-Z0-9_%/\\-+@:=&?#~.;]|[\\z00-\\zFF][^\\z00])");

    new Handler(
      *this,
      PROTO,
      DefaultEncodings,
      DefaultOptions,
      &Scanner::protoHitHandler
    );
  }

  void Scanner::initScan(const scanner_params& sp) {
    RFC822_Recorder = sp.fs.get_name("rfc822");
    Email_Recorder = sp.fs.get_name("email");
    Domain_Recorder = sp.fs.get_name("domain");
    Ether_Recorder = sp.fs.get_name("ether");
    URL_Recorder = sp.fs.get_name("url");
  }

  void Scanner::rfc822HitHandler(const LG_SearchHit& hit, const scanner_params& sp, const recursion_control_block& rcb) {
    RFC822_Recorder->write_buf(sp.sbuf, hit.Start, hit.End - hit.Start);
  }

  void Scanner::emailHitHandler(const LG_SearchHit& hit, const scanner_params& sp, const recursion_control_block& rcb) {
    const size_t len = (hit.End - 1) - hit.Start;

    const char* matchStart = reinterpret_cast<const char*>(sp.sbuf.buf) + hit.Start;

    if (validate_email(matchStart)) {
      Email_Recorder->write_buf(sp.sbuf, hit.Start, len);
      const size_t domain_off = find_domain_in_email(matchStart, len);
      if (domain_off > 0) {
        Domain_Recorder->write_buf(sp.sbuf, hit.Start + domain_off, len - domain_off);
      }
    }
  }

  void Scanner::ipaddrHitHandler(const LG_SearchHit& hit, const scanner_params& sp, const recursion_control_block& rcb) {
    // Get 8 characters of left context, right-justified
    const int c0 = max((int) hit.Start + 1 - 8, 0);
    const string context = string(" ", 8 - (hit.Start + 1 - c0)) +
      sp.sbuf.substr(c0, hit.Start + 1);

/*
    const int c0 = max((int) hit.Start - 8, 0);
    ostreamstring ss;
    ss << right << setw(8) << setfill(" ") << sp.sbuf.substr(c0, hit.Start);
    const string context(ss.str());
*/

// FIXME: this is horrid
    // Now have some rules for ignoring
    if (
      isalnum(context[7]) ||
      (context[7] == '.' || context[7] == '-' || context[7] == '+') ||
      (ishexnumber(context[4]) && ishexnumber(context[5]) && ishexnumber(context[6]) && context[7] == '}') ||
      (context.find("v.", 5)  != string::npos) ||
      (context.find("v ", 5)  != string::npos) ||
      (context.find("rv:", 5) != string::npos) || /* rv:1.9.2.8 as in Mozilla */
      (context.find(">=", 4)  != string::npos) || /* >= 1.8.0.10 */
      (context.find("<=", 4)  != string::npos) || /* <= 1.8.0.10 */
      (context.find("<<", 4)  != string::npos) || /* <= 1.8.0.10 */
      (context.find("ver", 4) != string::npos) ||
      (context.find("Ver", 4) != string::npos) ||
      (context.find("VER", 4) != string::npos) ||
      (context.find("rsion")  != string::npos) ||
      (context.find("ion=")   != string::npos) ||
      (context.find("PSW/")   != string::npos) ||  /* PWS/1.5.19.3 ... */
      (context.find("flash=") != string::npos) || /* flash= */
      (context.find("stone=") != string::npos) || /* Milestone= */
      (context.find("NSS", 4) != string::npos) ||
      (context.find("/2001,") != string::npos) || /* /2001,3.60.50.8 */
      (context.find("TI_SZ")  != string::npos) ||  /* %REG_MULTI_SZ%, */
      (sp.sbuf[hit.Start+1] == '0' && sp.sbuf[hit.Start+2] == '.')
    ) {
      // ignore
    }
    else {
      Domain_Recorder->write_buf(sp.sbuf, hit.Start+1, hit.End-hit.Start-2);
    }
  }

  void Scanner::etherHitHandler(const LG_SearchHit& hit, const scanner_params& sp, const recursion_control_block& rcb) {
    const size_t pos = hit.Start + 1;
    const size_t len = hit.End - pos;
    if (valid_ether_addr(sp.sbuf, pos)){
      Ether_Recorder->write_buf(sp.sbuf, pos, len);
    }
  }

  void Scanner::protoHitHandler(const LG_SearchHit& hit, const scanner_params& sp, const recursion_control_block& rcb) {
    // for reasons that aren't clear, there are a lot of net protocols that
    // have an http://domain in them followed by numbers. So this counts the
    // number of slashes and if it is only 2 the size is pruned until the
    // last character is a letter
    const int slash_count = count(
      sp.sbuf.buf + hit.Start,
      sp.sbuf.buf + hit.End, '/'
    );

    int feature_len = (hit.End - 1) - hit.Start;

    if (slash_count == 2) {
      while (feature_len > 0 && !isalpha(sp.sbuf[hit.Start+feature_len-1])) {
        --feature_len;
      }
    }

    URL_Recorder->write_buf(sp.sbuf, hit.Start, feature_len); // record the URL

    size_t domain_len = 0;
    size_t domain_start = find_domain_in_url(sp.sbuf.buf + hit.Start, feature_len, &domain_len);  // find the start of domain?
    if (domain_start > 0 && domain_len > 0) {
      Domain_Recorder->write_buf(sp.sbuf, hit.Start + domain_start, domain_len);
    }
  }

  Scanner TheScanner;
}

extern "C"
void scan_email_lg(const class scanner_params &sp, const recursion_control_block &rcb) {
  scan_lg(email::TheScanner, sp, rcb);
}

#endif // HAVE_LIBLIGHTGREP

