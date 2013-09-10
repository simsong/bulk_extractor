#include <string>

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
