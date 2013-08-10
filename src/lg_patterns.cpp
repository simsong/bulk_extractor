#include <iostream>
#include <string>

namespace accts {
  //
  // subpatterns
  //

  const char END[] = "([^0-9e.]|(\\.[^0-9]))";
  const char BLOCK[] = "[0-9]{4}";
  const char DELIM[] = "([- ])";
  const char DB[] = "({BLOCK}{DELIM})";
  const char SDB[] = "([45][0-9][0-9][0-9]{DELIM})";
  const char TDEL[] = "([ /.-])";

  const char PHONETEXT[] = "([^a-z](tel[.ephon]*|(fax)|(facsimile)|DSN|telex|TTD|mobile|cell)):?";

  // FIXME: post-2009?! 
  const char YEAR[] = "((19[0-9][0-9])|(20[01][0-9]))";
  const char MONTH[] = "(Jan(uary)?|Feb(ruary)?|Mar(ch)?|Apr(il)?|May|Jun(e)?|Jul(y)?|Aug(ust)?|Sep(tember)?|Oct(ober)?|Nov(ember)?|Dec(ember)?|0?1|0?2|0?3|0?4|0?5|0?6|0?7|0?8|0?9|10|11|12)";
  const char DAY[] = "([0-9]|[0-2][0-9]|30|31)";

  const char SYEAR[] = "([0-9][0-9])";
  const char SMONTH[] = "([0-1][0-2])";
 
  const char DATEA[] = "({YEAR}-{MONTH}-{DAY})";
  const char DATEB[] = "({YEAR}[/]{MONTH}[/]{DAY})";
  const char DATEC[] = "{DAY}[ ]{MONTH}[ ]{YEAR})";
  const char DATED[] = "({MONTH}[ ]{DAY}[, ]+{YEAR})";

  const char DATEFORMAT[] = "({DATEA}|{DATEB}|{DATEC}|{DATED})";

  //
  // patterns
  //

  /* #### #### #### #### #### #### is definately not a CCN. */
  const char REGEX1[] = "[^0-9a-z]{DB}{DB}{DB}{DB}{DB}";

  /* #### #### #### #### --- most credit card numbers*/
  const char REGEX2[] = "[^0-9a-z]{SDB}{DB}{DB}{BLOCK}/{END}";

  /* 3### ###### ######### --- 15 digits beginning with 3 and funny space. */
  /* Must be american express... */ 
  const char REGEX3[] = "[^0-9a-z.][3][0-9][0-9][0-9]{DELIM}[0-9]{6}{DELIM}[0-9]{5}/{END}";

  /* 3### ###### ######### --- 15 digits beginning with 3 and funny space. */
  /* Must be american express... */ 
  const char REGEX4[] = "[^0-9a-z.][3]([0-9]{14})/{END}";

  /* ###############  13-19 numbers as a block beginning with a 4 or 5
   * followed by something that is not a digit.
   * Yes, CCNs can now be up to 19 digits long. 
   * http://www.creditcards.com/credit-card-news/credit-card-appearance-1268.php
   */  
  const char REGEX5[] = "[^0-9a-z.][4-6]([0-9]{15,18})/{END}";

  /* ;###############=YYMM101#+? --- track2 credit card data */
  /* {SYEAR}{SMONTH} */
  /* ;CCN=05061010000000000738? */
  const char REGEX6[] = "[^0-9a-z][4-6]([0-9]{15,18})={SYEAR}{SMONTH}101[0-9]{13}";

  /* US phone numbers without area code in parens */
  /* New addition: If proceeded by " ####? ####? " 
   * then do not consider this a phone number. We see a lot of that stuff in
   * PDF files.
   */
  const char REGEX7[] = "[^0-9a-z][0-9][0-9][0-9]{TDEL}[0-9][0-9][0-9]{TDEL}[0-9][0-9][0-9][0-9]/{END}";

  /* US phone number with parens, like (215) 555-1212 */
  const char REGEX8[] = "[^0-9a-z]\\([0-9][0-9][0-9]\\){TDEL}?[0-9][0-9][0-9]{TDEL}[0-9][0-9][0-9][0-9]/{END}";

  /* Generalized international phone numbers */
  const char REGEX9[] = "[^0-9a-z]\\+[0-9]{1,3}(({TDEL}[0-9][0-9][0-9]?){2,6})[0-9]{2,4}/{END}";

  /* Generalized number with prefix */
  const char REGEX10[] = "{PHONETEXT}[0-9/ .+]{7,18}";

  /* Generalized number with city code and prefix */
  const char REGEX11[] = "{PHONETEXT}[0-9 +]+[ ]?[(][0-9]{2,4}[)][ ]?[\\-0-9]{4,8}";

  /* Possible BitLocker Recovery Key (ASCII). */
  const char BITLOCKER_ASCII[] = "[^0-9]([0-9]{6}-){7}([0-9]{6})/[\\r\\n]";

  /* Possible BitLocker Recovery Key (UTF-16). */
  const char BITLOCKER_UTF16[] = "\\0((([0-9]\\0){6}-\\0){7}(([0-9]\\0){6}))/[^0-9]";

  /* Generalized international phone numbers */
  const char REGEX12[] = "fedex[^a-z]+[0-9][0-9][0-9][0-9][- ]?[0-9][0-9][0-9][0-9][- ]?[0-9]/{END}";

  const char REGEX13[] = "ssn:?[ \\t]+[0-9][0-9][0-9]-?[0-9][0-9]-?[0-9][0-9][0-9][0-9]/{END}";

  const char REGEX14[] = "dob:?[ \\t]+{DATEFORMAT}";

  /* 
   * Common box arrays found in PDF files
   * With more testing this can and will still be tweaked
   */
  const char PDF_BOX[] = "box[ ]?[\\[][0-9 -]{0,40}[\\]]";

  /*
   * Common rectangles found in PDF files
   *  With more testing this can and will still be tweaked
   */
  const char PDF_RECT[] = "[\\[][ ]?[0-9.-]{1,12}[ ][0-9.-]{1,12}[ ][0-9.-]{1,12}[ ][0-9.-]{1,12}[ ]?[\\]]";
}

namespace base16 {
  //
  // patterns
  //

  /* 
   * hex with junk before it.
   * {0,4} means we have 0-4 space characters
   * {6,}  means minimum of 6 hex bytes
   */
  const char HEX1[] = "[^0-9A-F](([0-9A-F][0-9A-F][ \\t\\n\\r]{0,4}){6,})/[^0-9A-F]";

  /* hex at the beginning of the file */
  const char HEX2[] = "^(([0-9A-F][0-9A-F][ \\t\\n\\r]{0,4}){6,})/[^0-9A-F]";
}

namespace email {
  //
  // subpatterns
  //

  const char INUM[] = "(([0-9])|([0-9][0-9])|(1[0-9][0-9])|(2[0-4][0-9])|(25[0-5]))";
  const char HEX[] = "([0-9a-f])";
  const char ALNUM[] = "[a-zA-Z0-9]";
  const char PC[] = "[ !#$%&'()*+,\\-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ\\[\\\\]^_`abcdefghijklmnopqrstuvwxyz{|}~\"]";

  const char TLD[] = "(AC|AD|AE|AERO|AF|AG|AI|AL|AM|AN|AO|AQ|AR|ARPA|AS|ASIA|AT|AU|AW|AX|AZ|BA|BB|BD|BE|BF|BG|BH|BI|BIZ|BJ|BL|BM|BN|BO|BR|BS|BT|BV|BW|BY|BZ|CA|CAT|CC|CD|CF|CG|CH|CI|CK|CL|CM|CN|CO|COM|COOP|CR|CU|CV|CX|CY|CZ|DE|DJ|DK|DM|DO|DZ|EC|EDU|EE|EG|EH|ER|ES|ET|EU|FI|FJ|FK|FM|FO|FR|GA|GB|GD|GE|GF|GG|GH|GI|GL|GM|GN|GOV|GP|GQ|GR|GS|GT|GU|GW|GY|HK|HM|HN|HR|HT|HU|ID|IE|IL|IM|IN|INFO|INT|IO|IQ|IR|IS|IT|JE|JM|JO|JOBS|JP|KE|KG|KH|KI|KM|KN|KP|KR|KW|KY|KZ|LA|LB|LC|LI|LK|LR|LS|LT|LU|LV|LY|MA|MC|MD|ME|MF|MG|MH|MIL|MK|ML|MM|MN|MO|MOBI|MP|MQ|MR|MS|MT|MU|MUSEUM|MV|MW|MX|MY|MZ|NA|NAME|NC|NE|NET|NF|NG|NI|NL|NO|NP|NR|NU|NZ|OM|ORG|PA|PE|PF|PG|PH|PK|PL|PM|PN|PR|PRO|PS|PT|PW|PY|QA|RE|RO|RS|RU|RW|SA|SB|SC|SD|SE|SG|SH|SI|SJ|SK|SL|SM|SN|SO|SR|ST|SU|SV|SY|SZ|TC|TD|TEL|TF|TG|TH|TJ|TK|TL|TM|TN|TO|TP|TR|TRAVEL|TT|TV|TW|TZ|UA|UG|UK|UM|US|UY|UZ|VA|VC|VE|VG|VI|VN|VU|WF|WS|YE|YT|YU|ZA|ZM|ZW)";

  const char EMAIL[] = "{ALNUM}[a-zA-Z0-9._%\\-+]{1,128}{ALNUM}@{ALNUM}[a-zA-Z0-9._%\\-]{1,128}\\.{TLD}";

  // FIXME: post-2009?! 
  const char YEAR[] =	"((19[6-9][0-9])|(20[0-1][0-9]))";
  const char DAYOFWEEK[] = "(Mon|Tue|Wed|Thu|Fri|Sat|Sun)";
  const char MONTH[] = "(Jan|Feb|Mar|Apr|May|Jun|Jul|Aug|Sep|Oct|Nov|Dec)";
  const char ABBREV[] = "(UT|GMT|EST|EDT|CST|CDT|MST|MDT|PST|PDT|Z|A|M|N|Y)";

  const char U_TLD1[] = "(A\\0C\\0|A\\0D\\0|A\\0E\\0|A\\0E\\0R\\0O\\0|A\\0F\\0|A\\0G\\0|A\\0I\\0|A\\0L\\0|A\\0M\\0|A\\0N\\0|A\\0O\\0|A\\0Q\\0|A\\0R\\0|A\\0R\\0P\\0A\\0|A\\0S\\0|A\\0S\\0I\\0A\\0|A\\0T\\0|A\\0U\\0|A\\0W\\0|A\\0X\\0|A\\0Z\\0|B\\0A\\0|B\\0B\\0|B\\0D\\0|B\\0E\\0|B\\0F\\0|B\\0G\\0|B\\0H\\0|B\\0I\\0|B\\0I\\0Z\\0|B\\0J\\0|B\\0L\\0|B\\0M\\0|B\\0N\\0|B\\0O\\0|B\\0R\\0|B\\0S\\0|B\\0T\\0|B\\0V\\0|B\\0W\\0|B\\0Y\\0|B\\0Z\\0|C\\0A\\0|C\\0A\\0T\\0|C\\0C\\0|C\\0D\\0|C\\0F\\0|C\\0G\\0|C\\0H\\0|C\\0I\\0|C\\0K\\0|C\\0L\\0|C\\0M\\0|C\\0N\\0|C\\0O\\0|C\\0O\\0M\\0|C\\0O\\0O\\0P\\0|C\\0R\\0|C\\0U\\0|C\\0V\\0|C\\0X\\0|C\\0Y\\0|C\\0Z\\0)";
  const char U_TLD2[] = "(D\\0E\\0|D\\0J\\0|D\\0K\\0|D\\0M\\0|D\\0O\\0|D\\0Z\\0|E\\0C\\0|E\\0D\\0U\\0|E\\0E\\0|E\\0G\\0|E\\0H\\0|E\\0R\\0|E\\0S\\0|E\\0T\\0|E\\0U\\0|F\\0I\\0|F\\0J\\0|F\\0K\\0|F\\0M\\0|F\\0O\\0|F\\0R\\0|G\\0A\\0|G\\0B\\0|G\\0D\\0|G\\0E\\0|G\\0F\\0|G\\0G\\0|G\\0H\\0|G\\0I\\0|G\\0L\\0|G\\0M\\0|G\\0N\\0|G\\0O\\0V\\0|G\\0P\\0|G\\0Q\\0|G\\0R\\0|G\\0S\\0|G\\0T\\0|G\\0U\\0|G\\0W\\0|G\\0Y\\0|H\\0K\\0|H\\0M\\0|H\\0N\\0|H\\0R\\0|H\\0T\\0|H\\0U\\0|I\\0D\\0|I\\0E\\0|I\\0L\\0|I\\0M\\0|I\\0N\\0|I\\0N\\0F\\0O\\0|I\\0N\\0T\\0|I\\0O\\0|I\\0Q\\0|I\\0R\\0|I\\0S\\0|I\\0T\\0|J\\0E\\0|J\\0M\\0|J\\0O\\0|J\\0O\\0B\\0S\\0|J\\0P\\0|K\\0E\\0|K\\0G\\0|K\\0H\\0|K\\0I\\0|K\\0M\\0|K\\0N\\0|K\\0P\\0|K\\0R\\0|K\\0W\\0|K\\0Y\\0|K\\0Z\\0)";
  const char U_TLD3[] = "(L\\0A\\0|L\\0B\\0|L\\0C\\0|L\\0I\\0|L\\0K\\0|L\\0R\\0|L\\0S\\0|L\\0T\\0|L\\0U\\0|L\\0V\\0|L\\0Y\\0|M\\0A\\0|M\\0C\\0|M\\0D\\0|M\\0E\\0|M\\0F\\0|M\\0G\\0|M\\0H\\0|M\\0I\\0L\\0|M\\0K\\0|M\\0L\\0|M\\0M\\0|M\\0N\\0|M\\0O\\0|M\\0O\\0B\\0I\\0|M\\0P\\0|M\\0Q\\0|M\\0R\\0|M\\0S\\0|M\\0T\\0|M\\0U\\0|M\\0U\\0S\\0E\\0U\\0M\\0|M\\0V\\0|M\\0W\\0|M\\0X\\0|M\\0Y\\0|M\\0Z\\0|N\\0A\\0|N\\0A\\0M\\0E\\0|N\\0C\\0|N\\0E\\0|N\\0E\\0T\\0|N\\0F\\0|N\\0G\\0|N\\0I\\0|N\\0L\\0|N\\0O\\0|N\\0P\\0|N\\0R\\0|N\\0U\\0|N\\0Z\\0|O\\0M\\0|O\\0R\\0G\\0|P\\0A\\0|P\\0E\\0|P\\0F\\0|P\\0G\\0|P\\0H\\0|P\\0K\\0|P\\0L\\0|P\\0M\\0|P\\0N\\0|P\\0R\\0|P\\0R\\0O\\0|P\\0S\\0|P\\0T\\0|P\\0W\\0|P\\0Y\\0)";
  const char U_TLD4[] = "(Q\\0A\\0|R\\0E\\0|R\\0O\\0|R\\0S\\0|R\\0U\\0|R\\0W\\0|S\\0A\\0|S\\0B\\0|S\\0C\\0|S\\0D\\0|S\\0E\\0|S\\0G\\0|S\\0H\\0|S\\0I\\0|S\\0J\\0|S\\0K\\0|S\\0L\\0|S\\0M\\0|S\\0N\\0|S\\0O\\0|S\\0R\\0|S\\0T\\0|S\\0U\\0|S\\0V\\0|S\\0Y\\0|S\\0Z\\0|T\\0C\\0|T\\0D\\0|T\\0E\\0L\\0|T\\0F\\0|T\\0G\\0|T\\0H\\0|T\\0J\\0|T\\0K\\0|T\\0L\\0|T\\0M\\0|T\\0N\\0|T\\0O\\0|T\\0P\\0|T\\0R\\0|T\\0R\\0A\\0V\\0E\\0L\\0|T\\0T\\0|T\\0V\\0|T\\0W\\0|T\\0Z\\0|U\\0A\\0|U\\0G\\0|U\\0K\\0|U\\0M\\0|U\\0S\\0|U\\0Y\\0|U\\0Z\\0|V\\0A\\0|V\\0C\\0|V\\0E\\0|V\\0G\\0|V\\0I\\0|V\\0N\\0|V\\0U\\0|W\\0F\\0|W\\0S\\0|Y\\0E\\0|Y\\0T\\0|Y\\0U\\0|Z\\0A\\0|Z\\0M\\0|Z\\0W\\0)";

  //
  // patterns
  //

  const char DATE[] = "{DAYOFWEEK},[ \\t\\n]+[0-9]{1,2}[ \\t\\n]+{MONTH}[ \\t\\n]+{YEAR}[ \\t\\n]+[0-2][0-9]:[0-5][0-9]:[0-5][0-9][ \\t\\n]+([+-][0-2][0-9][0314][05]|{ABBREV})";

  const char MESSAGE_ID[] = "Message-ID:[ \\t\\n]?<{PC}{1,80}>";
  
  const char SUBJECT[] = "Subject:[ \\t]?({PC}{1,80})";

  const char COOKIE[] = "Cookie:[ \\t]?({PC}{1,80})";

  const char HOST[] = "Host:[ \\t]?([a-zA-Z0-9._]{1,64})";

  const char ADDR[] = "{EMAIL}/[^a-zA-Z]";

  /* Not an IP address, but could generate a false positive */
  const char NON_IP_1[] = "{INUM}?\\.{INUM}\\.{INUM}\\.{INUM}\\.{INUM}";

  /* Also not an IP address, but could generate a false positive */
  const char NON_IP_2[] = "[0-9][0-9][0-9][0-9]\\.{INUM}\\.{INUM}\\.{INUM}";

  /* Numeric IP addresses. Get the context before and throw away some things */
  const char IP[] = "{INUM}\\.{INUM}\\.{INUM}\\.{INUM}/[^0-9\\-.+A-Z_]";

  /* found a possible MAC address! */
  const char MAC[] = "[^0-9A-Z:]{HEX}{HEX}:{HEX}{HEX}:{HEX}{HEX}:{HEX}{HEX}:{HEX}{HEX}:{HEX}{HEX}/[^0-9A-Z:]";

  // for reasons that aren't clear, there are a lot of net protocols that have
  // an http://domain in them followed by numbers. So this counts the number of
  // slashes and if it is only 2 the size is pruned until the last character
  // is a letter
  const char PROTO[] = "((https?)|afp|smb)://[a-zA-Z0-9_%/\\-+@:=&?#~.;]{1,384}";
 
  const char EMAIL_UTF16LE[] = "[a-zA-Z0-9]\\0([a-zA-Z0-9._%\\-+]\\0){1,128}@\\0([a-zA-Z0-9._%\\-]\\0){1,128}\\.\\0({U_TLD1}|{U_TLD2}|{U_TLD3}|{U_TLD4})/[^a-zA-Z]|([^][^\\0])";

  const char HTTP_UTF16LE[] = "h\\0t\\0t\\0p\\0(s\\0)?:\\0([a-zA-Z0-9_%/\\-+@:=&?#~.;]\\0){1,128}/[^a-zA-Z0-9_%/\\-+@:=&?#~.;]|([^][^\\0])";
}

namespace gps {
  //
  // subpatterns
  //

  const char LATLON[] = "(-?[0-9]{1,3}[.][0-9]{6,8})";
  const char ELEV[] =	"(-?[0-9]{1,6}[.][0-9]{0,3})";

  //
  // patterns
  //

  const char TRKPT[] = "<trkpt lat=\"{LATLON}\" lon=\"{LATLON}\"";
  
  const char ELE[] = "<ele>{ELEV}</ele>";

  const char TIME[] = "<time>[0-9][0-9][0-9][0-9]-[0-9][0-9]-[0-9][0-9][ T][0-9][0-9]:[0-9][0-9]:[0-9][0-9](Z|([-+][0-9.]))</time>";

  const char GPXTPX_SPEED[] = "<gpxtpx:speed>{ELEV}</gpxtpx:speed>";

  const char GPXTPX_COURSE[] = "<gpxtpx:course>{ELEV}</gpxtpx:course>";
}

namespace httpheader {
  //
  // subpatterns
  //

  const char PC[] = "[ !#$%&'()*+,\\-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ\\[\\\\]^_`abcdefghijklmnopqrstuvwxyz{|}~\"]";

  const char XPC[] = "[ !#$%&'()*+,\\-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ\\[\\\\]^_`abcdefghijklmnopqrstuvwxyz{|}~]"; 
  
  /*
   * RFC 2616, Page 12
   */
  /* Account for over-zealously translated line breaks */
  /* HTTP_LWS - Linear White Space (new line and a whitespace character) */
  const char HTTP_CRLF[] = "(\\x0D\\x0A|\\x0A)";
  const char HTTP_LWS[] = "{HTTP_CRLF}[ \\t]";

  /*
   * Keeping it simple: no HTTP_CTEXT, HTTP_QUOTED_PAIR, or keeping count
   * of parentheses. The distinguishing part of COMMENTs is they are
   * allowed to have line breaks, if followed by whitespace.
   *
   * TODO Might still need to account for RFC 2407.
   */
  const char HTTP_COMMENT[] = "({PC}|{HTTP_LWS}|[\\t])";

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
  const char SERVER_OR_UA[] = "(Server|User-Agent):[ \\t]?({PC}{1,80})";
  
  /*
   * RFC 2616, Section 14.23
   */
  const char HOST[] = "Host:[ \\t]?([a-zA-Z0-9._:]{1,256})";

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
  const char HEADERS_1[] = "(Accept|Accept-Ranges|Authorization|Cache-Control|Content-Location|Etag|Expect|Keep-Alive|If-Match|If-None-Match|If-Range|Pragma|Proxy-Authenticate|Proxy-Authorization|Referer|TE|Transfer-Encoding|Warning|WWW-Authenticate):[ \\t]?({PC}){1,80}";

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
  const char HEADERS_2[] = "(Accept-Charset|Accept-Encoding|Accept-Language|Age|Allow|Connection|Content-Encoding|Content-Language|Content-MD5|Content-Range|Content-Type|Cookie|Date|From|If-Modified-Since|If-Unmodified-Since|Last-Modified|Range|Retry-After|Set-Cookie|Trailer|Upgrade|Vary):[ \\t]?({XPC}){1,80}";

  const char VIA[] = "Via:[ \\t]?{HTTP_COMMENT}{1,256}";

  /*
   * RFC 2616, Sections 14.13 and 14.31
   */
  const char HEADERS_3[] = "(Content-Length|Max-Forwards):[ \\t]?[0-9]{1,12}";
}

int main(int argc, char** argv) {
  return 0;
}
