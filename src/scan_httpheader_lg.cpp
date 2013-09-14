#include <string>

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
