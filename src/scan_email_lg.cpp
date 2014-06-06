#include "config.h"

// if liblightgrep isn't present, compiles to nothing
#ifdef HAVE_LIBLIGHTGREP

#include <algorithm>
#include <functional>
#include <string>

#include "be13_api/bulk_extractor_i.h"
#include "histogram.h"
#include "pattern_scanner.h"
#include "pattern_scanner_utils.h"
#include "utils.h"

using namespace std;

namespace email {
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
  const string ABBREV("(UTC?|GMT|EST|EDT|CST|CDT|MST|MDT|PST|PDT|[ZAMNY])");

  //
  // helper functions
  //

  // NB: It is very important *not* to use functions expecting C strings
  // or std::strings on hit data, as hit data could contain internal null
  // bytes.

  /** return the offset of the domain in an email address.
   * returns buflen + 1 if the domain is not found.
   * the domain extends to the end of the email address
   */
  inline size_t find_domain_in_email(const uint8_t* buf, size_t buflen) {
    return find(buf, buf + buflen, '@') - buf + 1;
  }

  template <typename T>
  inline size_t find_domain_in_url(const T* buf, size_t buflen, size_t& domain_len) {
    const T* dbeg = search_n(buf, buf + buflen, 2, '/') + 2;
    if (dbeg < buf + buflen) {
      const T stop[] = { '/', ':' };
      const T* dend = find_first_of(dbeg, buf + buflen, stop, stop + 2);
      domain_len = dend - dbeg;
      return dbeg - buf;
    }

    return buflen;
  }

  bool valid_ether_addr(const uint8_t* buf) {
    if (memcmp((const uint8_t *)"00:00:00:00:00:00", buf, 17) == 0) {
      return false;
    }

    if (memcmp((const uint8_t *)"00:11:22:33:44:55", buf, 17) == 0) {
      return false;
    }

    /* Perform a quick histogram analysis.
     * For each group of characters, create a value based on the two digits.
     * There is no need to convert them to their 'actual' value.
     * Don't accept a histogram that has 3 values. That could be
     * 11:11:11:11:22:33
     * Require 4, 5 or 6.
     * If we have 4 or more distinct values, then treat it good.
     * Otherwise its is some pattern we don't want.
     */
    set<uint16_t> ctr;
    for (uint32_t i = 0; i < 6; ++i) {  // loop over each group
      // create a unique value of the two characters
      ctr.insert((buf[i*3] << 8) + buf[i*3+1]);
    }

    return ctr.size() >= 4;
  }

  template <typename T>
  bool valid_ipaddr(const T* leftguard, const T* hit) {
    // copy up to 'window' preceding Ts into context array
    static const ssize_t window = 8;
    T context[window] = { ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ' };
    const ssize_t diff = min(hit - leftguard, window);
    copy(hit - diff, hit, context + window - diff);

    if (
      isalnum(context[7]) ||
      context[7] == '.' ||
      context[7] == '-' ||
      context[7] == '+' ||
      (ishexnumber(context[4]) && ishexnumber(context[5]) &&
       ishexnumber(context[6]) && context[7] == '}') ||
      (*hit == '0' && *(hit + 1) == '.'))
    {
      // ignore
      return false;
    }

    static const struct {
      size_t pos;
      const char* str;
    } checks[] = {
      { 5, "v."     },
      { 5, "v "     },
      { 5, "rv:"    },  // rv:1.9.2.8 as in Mozilla
      { 4, ">="     },  // >= 1.8.0.10
      { 4, "<="     },  // <= 1.8.0.10
      { 4, "<<"     },  // << 1.8.0.10
      { 4, "ver"    },
      { 4, "Ver"    },
      { 4, "VER"    },
      { 0, "rsion"  },
      { 0, "ion="   },
      { 0, "PSW/"   },  // PWS/1.5.19.3 ...
      { 0, "flash=" },  // flash=
      { 0, "stone=" },  // Milestone=
      { 4, "NSS"    },
      { 0, "/2001," },  // /2001,3.60.50.8
      { 0, "TI_SZ"  }   // %REG_MULTI_SZ%,
    };

    for (size_t i = 0; i < sizeof(checks)/sizeof(checks[0]); ++i) {
      if (search(
        context + checks[i].pos,
        context + 8, checks[i].str,
        checks[i].str + strlen(checks[i].str)
      ) != context + 8) {
        return false;
      }
    }

    return true;
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

    void emailUTF16LEHitHandler(const LG_SearchHit& hit, const scanner_params& sp, const recursion_control_block& rcb);

    void ipaddrHitHandler(const LG_SearchHit& hit, const scanner_params& sp, const recursion_control_block& rcb);

    void ipaddrUTF16LEHitHandler(const LG_SearchHit& hit, const scanner_params& sp, const recursion_control_block& rcb);

    void etherHitHandler(const LG_SearchHit& hit, const scanner_params& sp, const recursion_control_block& rcb);

    void etherUTF16LEHitHandler(const LG_SearchHit& hit, const scanner_params& sp, const recursion_control_block& rcb);

    void protoHitHandler(const LG_SearchHit& hit, const scanner_params& sp, const recursion_control_block& rcb);

    void protoUTF16LEHitHandler(const LG_SearchHit& hit, const scanner_params& sp, const recursion_control_block& rcb);

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

    const string DATE(DAYOFWEEK + ",[ \\t\\n\\r]+[0-9]{1,2}[ \\t\\n\\r]+" + MONTH + "[ \\t\\n\\r]+" + YEAR + "[ \\t\\n\\r]+[0-2][0-9]:[0-5][0-9]:[0-5][0-9][ \\t\\n\\r]+([+-][0-2][0-9][0314][05]|" + ABBREV + ")");

    new Handler(
      *this,
      DATE,
      DefaultEncodings,
      DefaultOptions,
      &Scanner::rfc822HitHandler
    );

    const string MESSAGE_ID("Message-ID:([ \\t\\n]|\\r\\n)?<" + PC + "+>");

    new Handler(
      *this,
      MESSAGE_ID,
      DefaultEncodings,
      DefaultOptions,
      &Scanner::rfc822HitHandler
    );

    const string SUBJECT("Subject:[ \\t]?" + PC + "+");

    new Handler(
      *this,
      SUBJECT,
      DefaultEncodings,
      DefaultOptions,
      &Scanner::rfc822HitHandler
    );

    const string COOKIE("Cookie:[ \\t]?" + PC + "+");

    new Handler(
      *this,
      COOKIE,
      DefaultEncodings,
      DefaultOptions,
      &Scanner::rfc822HitHandler
    );

    const string HOST("Host:[ \\t]?[a-zA-Z0-9._]+");

    new Handler(
      *this,
      HOST,
      DefaultEncodings,
      DefaultOptions,
      &Scanner::rfc822HitHandler
    );

    // FIXME: trailing context
//    const string EMAIL(ALNUM + "[a-zA-Z0-9._%\\-+]+" + ALNUM + "@" + ALNUM + "[a-zA-Z0-9._%\\-]+\\." + TLD + "[^\\z41-\\z5A\\z61-\\z7A]");
    const string EMAIL(ALNUM + "(\\.?[a-zA-Z0-9_%\\-+])+\\.?" + ALNUM + "@" + ALNUM + "(\\.?[a-zA-Z0-9_%\\-])+\\." + TLD + "[^\\z41-\\z5A\\z61-\\z7A]");

    new Handler(
      *this,
      EMAIL,
      OnlyUTF8Encoding,
      DefaultOptions,
      &Scanner::emailHitHandler
    );

    new Handler(
      *this,
      EMAIL,
      OnlyUTF16LEEncoding,
      DefaultOptions,
      &Scanner::emailUTF16LEHitHandler
    );

    // FIXME: leading context
    // FIXME: trailing context
    // Numeric IP addresses. Get the context before and throw away some things
    const string IP("[^\\z30-\\z39\\z2E]" + INUM + "(\\." + INUM + "){3}[^\\z30-\\z39\\z2B\\z2D\\z2E\\z41-\\z5A\\z5F\\z61-\\z7A]");

    new Handler(
      *this,
      IP,
      OnlyUTF8Encoding,
      DefaultOptions,
      &Scanner::ipaddrHitHandler
    );

    const string IP_UTF16LE("([^\\z30-\\z39\\z2E]\\z00|[^\\z00])" + INUM + "(\\." + INUM + "){3}[^\\z30-\\z39\\z2B\\z2D\\z2E\\z41-\\z5A\\z5F\\z61-\\z7A]");

    new Handler(
      *this,
      IP_UTF16LE,
      OnlyUTF16LEEncoding,
      DefaultOptions,
      &Scanner::ipaddrUTF16LEHitHandler
    );

    // FIXME: leading context
    // FIXME: trailing context
    // found a possible MAC address!
    const string MAC("[^\\z30-\\z39\\z3A\\z41-\\z5A\\z61-\\z7A]" + HEX + "{2}(:" + HEX + "{2}){5}[^\\z30-\\z39\\z3A\\z41-\\z5A\\z61-\\z7A]");

    new Handler(
      *this,
      MAC,
      OnlyUTF8Encoding,
      DefaultOptions,
      &Scanner::etherHitHandler
    );

    const string MAC_UTF16LE("([^\\z30-\\z39\\z3A\\z41-\\z5A\\z61-\\z7A]\\z00|[^\\z00])" + HEX + "{2}(:" + HEX + "{2}){5}[^\\z30-\\z39\\z3A\\z41-\\z5A\\z61-\\z7A]");

    new Handler(
      *this,
      MAC,
      OnlyUTF16LEEncoding,
      DefaultOptions,
      &Scanner::etherUTF16LEHitHandler
    );

    const string PROTO("(https?|afp|smb)://[a-zA-Z0-9_%/\\-+@:=&?#~.;]+");

    new Handler(
      *this,
      PROTO,
      OnlyUTF8Encoding,
      DefaultOptions,
      &Scanner::protoHitHandler
    );

    new Handler(
      *this,
      PROTO,
      OnlyUTF16LEEncoding,
      DefaultOptions,
      &Scanner::protoUTF16LEHitHandler
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
    const uint8_t* matchStart = sp.sbuf.buf + hit.Start;

    Email_Recorder->write_buf(sp.sbuf, hit.Start, len);
    const size_t domain_off = find_domain_in_email(matchStart, len);
    if (domain_off < len) {
      Domain_Recorder->write_buf(sp.sbuf, hit.Start + domain_off, len - domain_off);
    }
  }

  void Scanner::emailUTF16LEHitHandler(const LG_SearchHit& hit, const scanner_params& sp, const recursion_control_block& rcb) {
    const size_t len = (hit.End - 1) - hit.Start;
    const uint8_t* matchStart = sp.sbuf.buf + hit.Start;

    Email_Recorder->write_buf(sp.sbuf, hit.Start, len);
    const size_t domain_off = find_domain_in_email(matchStart, len) + 1;
    if (domain_off < len) {
      Domain_Recorder->write_buf(sp.sbuf, hit.Start + domain_off, len - domain_off);
    }
  }

  void Scanner::ipaddrHitHandler(const LG_SearchHit& hit, const scanner_params& sp, const recursion_control_block& rcb) {
    if (valid_ipaddr(sp.sbuf.buf, sp.sbuf.buf + hit.Start + 1)) {
      Domain_Recorder->write_buf(sp.sbuf, hit.Start+1, hit.End - hit.Start - 2);
    }
  }

  void Scanner::ipaddrUTF16LEHitHandler(const LG_SearchHit& hit, const scanner_params& sp, const recursion_control_block& rcb) {
    const size_t pos = hit.Start + (*(sp.sbuf.buf+hit.Start+1) == '\0' ? 2 : 1);
    const size_t len = (hit.End - 1) - pos;
    // this assumes sp.sbuf.pos will never be an odd memory address...
    // if pos is odd, add 1 to sbuf.buf and use it as a leftmost guard
    const uint16_t* leftguard(reinterpret_cast<const uint16_t*>(sp.sbuf.buf + ((pos & 0x01) == 1 ? 1: 0)));
    if (valid_ipaddr(leftguard, reinterpret_cast<const uint16_t*>(sp.sbuf.buf + pos))) {
      Domain_Recorder->write_buf(sp.sbuf, pos, len);
    }
  }

  void Scanner::etherHitHandler(const LG_SearchHit& hit, const scanner_params& sp, const recursion_control_block& rcb) {
    const size_t pos = hit.Start + 1;
    const size_t len = (hit.End - 1) - pos;
    if (valid_ether_addr(sp.sbuf.buf+pos)){
      Ether_Recorder->write_buf(sp.sbuf, pos, len);
    }
  }

  void Scanner::etherUTF16LEHitHandler(const LG_SearchHit& hit, const scanner_params& sp, const recursion_control_block& rcb) {
    const size_t pos = hit.Start + (*(sp.sbuf.buf+hit.Start+1) == '\0' ? 2 : 1);
    const size_t len = (hit.End -1) - pos;

    const string ascii(low_utf16le_to_ascii(sp.sbuf.buf+pos, len));
    if (valid_ether_addr(reinterpret_cast<const uint8_t*>(ascii.c_str()))){
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

    size_t len = hit.End - hit.Start;

    if (slash_count == 2) {
      while (len > 0 && !isalpha(sp.sbuf[hit.Start+len-1])) {
        --len;
      }
    }

    URL_Recorder->write_buf(sp.sbuf, hit.Start, len);

    size_t domain_len = 0;
    size_t domain_off = find_domain_in_url(sp.sbuf.buf + hit.Start, len, domain_len);  // find the start of domain?
    if (domain_off < len && domain_len > 0) {
      Domain_Recorder->write_buf(sp.sbuf, hit.Start + domain_off, domain_len);
    }
  }

  void Scanner::protoUTF16LEHitHandler(const LG_SearchHit& hit, const scanner_params& sp, const recursion_control_block& rcb) {
    const int slash_count = count(
      sp.sbuf.buf + hit.Start,
      sp.sbuf.buf + hit.End, '/'
    );

    size_t len = hit.End - hit.Start;

    if (slash_count == 2) {
      while (len > 1 && !isalpha(sp.sbuf[hit.Start+len-2])) {
        len -= 2;
      }
    }

    URL_Recorder->write_buf(sp.sbuf, hit.Start, len);

    size_t domain_len = 0;
    size_t domain_off = find_domain_in_url(reinterpret_cast<const uint16_t*>(sp.sbuf.buf + hit.Start), len/2, domain_len);  // find the start of domain?
    domain_off *= 2;
    domain_len *= 2;
    if (domain_off < len && domain_len > 0) {
      Domain_Recorder->write_buf(sp.sbuf, hit.Start + domain_off, domain_len);
    }
  }

  Scanner TheScanner;
}

extern "C"
void scan_email_lg(const class scanner_params &sp, const recursion_control_block &rcb) {
  scan_lg(email::TheScanner, sp, rcb);
}

#endif // HAVE_LIBLIGHTGREP
