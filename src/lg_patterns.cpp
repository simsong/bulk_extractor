#include <algorithm>
#include <iostream>
#include <iterator>
#include <string>

namespace accts {
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

  // FIXME: leading context
  // FIXME: trailing context
  /* 3### ###### ######### --- 15 digits beginning with 3 and funny space. */
  /* Must be american express... */ 
  const std::string REGEX3("[^0-9a-z.]3[0-9]{3}" + DELIM + "[0-9]{6}" + DELIM + "[0-9]{5}" + END);

  // FIXME: leading context
  // FIXME: trailing context
  /* 3### ###### ######### --- 15 digits beginning with 3 and funny space. */
  /* Must be american express... */ 
  const std::string REGEX4("[^0-9a-z.]3[0-9]{14}" + END);

  // FIXME: leading context
  // FIXME: trailing context
  /* ###############  13-19 numbers as a block beginning with a 4 or 5
   * followed by something that is not a digit.
   * Yes, CCNs can now be up to 19 digits long. 
   * http://www.creditcards.com/credit-card-news/credit-card-appearance-1268.php
   */  
  const std::string REGEX5("[^0-9a-z.][4-6][0-9]{15,18}" + END);

  // FIXME: leading context
  /* ;###############=YYMM101#+? --- track2 credit card data */
  /* {SYEAR}{SMONTH} */
  /* ;CCN=05061010000000000738? */
  const std::string REGEX6("[^0-9a-z][4-6][0-9]{15,18}=" + SYEAR + SMONTH + "101[0-9]{13}");

  // FIXME: trailing context
  // FIXME: leading context
  /* US phone numbers without area code in parens */
  /* New addition: If proceeded by " ####? ####? " 
   * then do not consider this a phone number. We see a lot of that stuff in
   * PDF files.
   */
  const std::string REGEX7("[^0-9a-z]([0-9]{3}" + TDEL + "){2}[0-9]{4}" + END);

  // FIXME: trailing context
  // FIXME: leading context
  /* US phone number with parens, like (215) 555-1212 */
  const std::string REGEX8("[^0-9a-z]\\([0-9]{3}\\)" + TDEL + "?[0-9]{3}" + TDEL + "[0-9]{4}" + END);

  // FIXME: trailing context
  // FIXME: leading context
  /* Generalized international phone numbers */
  const std::string REGEX9("[^0-9a-z]\\+[0-9]{1,3}(" + TDEL + "[0-9]{2,3}){2,6}[0-9]{2,4}" + END);

  /* Generalized number with prefix */
  const std::string REGEX10(PHONETEXT + "[0-9/ .+]{7,18}");

  /* Generalized number with city code and prefix */
  const std::string REGEX11(PHONETEXT + "[0-9 +]+ ?\\([0-9]{2,4}\\) ?[\\-0-9]{4,8}");

  // FIXME: trailing context
  /* Generalized international phone numbers */
  const std::string REGEX12("fedex[^a-z]+([0-9]{4}[- ]?){2}[0-9]" + END);

  // FIXME: trailing context
  const std::string REGEX13("ssn:?[ \\t]+[0-9]{3}-?[0-9]{2}-?[0-9]{4}" + END);

  const std::string REGEX14("dob:?[ \\t]+" + DATEFORMAT);

  // FIXME: trailing context
  /* Possible BitLocker Recovery Key. */
  const std::string BITLOCKER("[^\\z30-\\z39]([0-9]{6}-){7}[0-9]{6}[^\\z30-\\z39]");

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

namespace base16 {
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
  const std::string HEX("[^0-9A-F]([0-9A-F]{2}[ \\t\\n\\r]{0,4}){6,}[^0-9A-F]");
}

namespace email {
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

  const std::string MESSAGE_ID("Message-ID:[ \\t\\n]?<" + PC + "{1,80}>");
  
  const std::string SUBJECT("Subject:[ \\t]?" + PC + "{1,80}");

  const std::string COOKIE("Cookie:[ \\t]?" + PC + "{1,80}");

  const std::string HOST("Host:[ \\t]?[a-zA-Z0-9._]{1,64}");

  // FIXME: trailing context
  const std::string EMAIL(ALNUM + "([a-zA-Z0-9._%\\-+]*?" + ALNUM + ")?@(" + ALNUM + "([a-zA-Z0-9\\-]*?" + ALNUM + ")?\\.)+" + TLD + "([^a-zA-Z]|[\\z00-\\zFF][^\\z00])");

  // FIXME: leading context
  // FIXME: trailing context
  /* Numeric IP addresses. Get the context before and throw away some things */
  const std::string IP("[^0-9.]" + INUM + "(\\." + INUM + "){3}[^0-9\\-.+A-Z_]");

  // FIXME: leading context
  // FIXME: trailing context
  // FIXME: should we be searching for all uppercase MAC addresses as well?
  /* found a possible MAC address! */
  const std::string MAC("[^0-9A-Z:]" + HEX + "{2}(:" + HEX + "{2}){5}[^0-9A-Z:]");

  // for reasons that aren't clear, there are a lot of net protocols that have
  // an http://domain in them followed by numbers. So this counts the number of
  // slashes and if it is only 2 the size is pruned until the last character
  // is a letter
  // FIXME: trailing context
  const std::string PROTO("(https?|afp|smb)://[a-zA-Z0-9_%/\\-+@:=&?#~.;]+([^a-zA-Z0-9_%/\\-+@:=&?#~.;]|[\\z00-\\zFF][^\\z00])");
}

namespace gps {
  //
  // subpatterns
  //

  const std::string LATLON("(-?[0-9]{1,3}\\.[0-9]{6,8})");
  const std::string ELEV("(-?[0-9]{1,6}\\.[0-9]{0,3})");

  //
  // patterns
  //

  const std::string TRKPT("<trkpt lat=\"" + LATLON + "\" lon=\"" + LATLON + '"');
  
  const std::string ELE("<ele>" + ELEV + "</ele>");

  const std::string TIME("<time>[0-9]{4}(-[0-9]{2}){2}[ T]([0-9]{2}:){2}[0-9]{2}(Z|([-+][0-9.]))</time>");

  const std::string GPXTPX_SPEED("<gpxtpx:speed>" + ELEV + "</gpxtpx:speed>");

  const std::string GPXTPX_COURSE("<gpxtpx:course>" + ELEV + "</gpxtpx:course>");
}

namespace httpheader {
  //
  // subpatterns
  //

  const std::string PC("[\\x20-\\x7E]");

  const std::string XPC("[\\x20-\\x7E--\"]");
  
  /*
   * RFC 2616, Page 12
   */
  /* Account for over-zealously translated line breaks */
  /* HTTP_LWS - Linear White Space (new line and a whitespace character) */
  const std::string HTTP_CRLF("(\\x0D?\\x0A)");
  const std::string HTTP_LWS(HTTP_CRLF + "[ \\t]");

  /*
   * Keeping it simple: no HTTP_CTEXT, HTTP_QUOTED_PAIR, or keeping count
   * of parentheses. The distinguishing part of COMMENTs is they are
   * allowed to have line breaks, if followed by whitespace.
   *
   * TODO Might still need to account for RFC 2407.
   */
  const std::string HTTP_COMMENT("(" + PC + "|" + HTTP_LWS + "|\\t)");

  //
  // patterns
  //

  /*
   * RFC 2616, Sections 14.38 and 14.43
   * These fields are allowed multi-line values (comments).
   *
   * For some reason, specifying the field value as:
   *   ({XPC}|{HTTP_LWS})+
   * causes the NFA rule set to explode to >32000 rules, making flex refuse
   * to compile.
   */
  const std::string SERVER_OR_UA("(Server|User-Agent):[ \\t]?" + PC + "{1,80}");
  
  /*
   * RFC 2616, Section 14.23
   */
  const std::string HOST("Host:[ \\t]?[a-zA-Z0-9._:]{1,256}");

  /*
   * These headers have a general set of characters allowed in their field
   * value, including double-quote.
   *
   * Keep-Alive is defined in RFC 2068, Section 19.7.1.1. Allowable tokens
   * seem to include doublequote, per "value" definition in RFC 2068,
   * Section 3.7.
   *
   * Authorization, Proxy-Authenticate, Proxy-Authorization and WWW-
   * Authenticate are defined in RFC 2617, not yet reviewed. Assuming PC
   * character set allowed for now.
   *
   * Content-Location, Location and Referer (RFC 2616, Sections 14.14, 14.30
   * and 14.36) have a URI as the field value.
   * SLG: Limited to 80 characters
   */
  const std::string HEADERS_1("(Accept|Accept-Ranges|Authorization|Cache-Control|Content-Location|Etag|Expect|Keep-Alive|If-Match|If-None-Match|If-Range|Pragma|Proxy-Authenticate|Proxy-Authorization|Referer|TE|Transfer-Encoding|Warning|WWW-Authenticate):[ \\t]?" + PC + "{1,80}");

  /*
   * These headers have a general set of characters allowed in their field
   * value, excluding double-quote.
   *
   * Date and If-Modified-Since reference RFCs 1123 and 850 (RFC 2616
   * Section 3.3.1), not yet reviewed. 
   * Double-quotes are assumed excluded.
   *
   * Set-Cookie: RFC 6265, Section 4.1.1, Page 9
   * This header field is allowed to be sent multiple times in the same
   * header.
   *
   * Cookie: RFC 6265, Section 4.2.1, Page 13
   * The cookie length does not seem to have a limit, but cookie stores should
   * be able to store at least 4096 bytes for a cookie [RFC 6265, Section 6.1].
   *
   * From: should contain an email address.
   */
  const std::string HEADERS_2("(Accept-Charset|Accept-Encoding|Accept-Language|Age|Allow|Connection|Content-Encoding|Content-Language|Content-MD5|Content-Range|Content-Type|Cookie|Date|From|If-Modified-Since|If-Unmodified-Since|Last-Modified|Range|Retry-After|Set-Cookie|Trailer|Upgrade|Vary):[ \\t]?" + XPC + "{1,80}");

  const std::string VIA("Via:[ \\t]?" + HTTP_COMMENT + "{1,256}");

  /*
   * RFC 2616, Sections 14.13 and 14.31
   */
  const std::string HEADERS_3("(Content-Length|Max-Forwards):[ \\t]?[0-9]{1,12}");
}

struct Pattern {
  std::string pat, enc;
};

std::ostream& operator<<(std::ostream& o, Pattern p) {
  return o << p.pat << "\t0\t0\t" << p.enc;
}

int main(int argc, char** argv) {

  const Pattern all[] = {
    { accts::REGEX1,            "ASCII" },
    { accts::REGEX2,            "ASCII" },
    { accts::REGEX3,            "ASCII" },
    { accts::REGEX4,            "ASCII" },
    { accts::REGEX5,            "ASCII" },
    { accts::REGEX6,            "ASCII" },
    { accts::REGEX7,            "ASCII" },
    { accts::REGEX8,            "ASCII" },
    { accts::REGEX9,            "ASCII" },
    { accts::REGEX10,           "ASCII" },
    { accts::REGEX11,           "ASCII" },
    { accts::REGEX12,           "ASCII" },
    { accts::REGEX13,           "ASCII" },
    { accts::REGEX14,           "ASCII" },
    { accts::BITLOCKER,         "ASCII" },
    { accts::PDF_BOX,           "ASCII" },
    { accts::PDF_RECT,          "ASCII" },
    { base16::HEX,              "ASCII" },
    { email::DATE,              "ASCII" },
    { email::MESSAGE_ID,        "ASCII" },
    { email::SUBJECT,           "ASCII" },
    { email::COOKIE,            "ASCII" },
    { email::HOST,              "ASCII" },
    { email::EMAIL,             "ASCII" },
    { email::IP,                "ASCII" },
    { email::MAC,               "ASCII" },
    { email::PROTO,             "ASCII" },
    { gps::TRKPT,               "ASCII" },
    { gps::ELE,                 "ASCII" },
    { gps::TIME,                "ASCII" },
    { gps::GPXTPX_SPEED,        "ASCII" },
    { gps::GPXTPX_COURSE,       "ASCII" },
/*
    { httpheader::SERVER_OR_UA, "ASCII" },
    { httpheader::HOST,         "ASCII" },
    { httpheader::HEADERS_1,    "ASCII" },
    { httpheader::HEADERS_2,    "ASCII" },
    { httpheader::VIA,          "ASCII" },
    { httpheader::HEADERS_3,    "ASCII" }
*/
  };

  std::copy(
    all, all + sizeof(all)/sizeof(all[0]),
    std::ostream_iterator<Pattern>(std::cout, "\n")
  );

  return 0;
}
