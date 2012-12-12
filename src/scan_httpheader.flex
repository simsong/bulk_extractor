%{
/*
 *  http://flex.sourceforge.net/manual/Cxx.html
 *
 *  This scanner has a singular focus on extracting HTTP headers by recognizing their fields.
 * 
 * http://stackoverflow.com/questions/921648/how-to-make-a-flex-lexical-scanner-to-read-utf-8-characters-input
 * http://xcybercloud.blogspot.com/2009/04/unicode-support-in-flex.html
 */

#include "config.h"
#include "be13_api/bulk_extractor_i.h"

#include <stdlib.h>
#include <string.h>

/* Needed for flex: */
#undef 	ECHO
#define	ECHO {}
#define YY_SKIP_YYWRAP
#define YY_DECL int MyHTTPHeaderFlexLexer::yylex()

class MyHTTPHeaderFlexLexer : public httpheaderFlexLexer {
      public:
      MyHTTPHeaderFlexLexer(const sbuf_t &sbuf_):sbuf(sbuf_){
	  this->pos		= 0;
          this->point		= 0;
	  this->eof             = false;
      }
      const sbuf_t &sbuf;
      size_t pos;
      size_t point;
      bool eof;
      class feature_recorder *httpheader_recorder;
      virtual int LexerInput(char *buf,int max_size_){  /* signature can't be changed */
      	      if(eof) return 0;
      	      unsigned int max_size = max_size_ > 0 ? max_size_ : 0;
      	      int count = 0;
      	      if(max_size + point > sbuf.bufsize){
	         max_size = sbuf.bufsize - point;
              }
	      while(max_size>0){
	         *buf = (char)sbuf.buf[point];
		 buf++;
		 point++;
		 max_size--;
		 count++;
  	      }
	      return count;
      }
      virtual int yylex();
};

int httpheaderFlexLexer::yylex(){return 0;}


/**
 *  note below:
 * HTTP headers are specified to be fairly proper-cased,
 * and some of the words are quite common, so we are not case insensitive.
 */

%}

%option c++
%option noyywrap
%option 8bit
%option batch
%option pointer
%option noyymore
%option prefix="httpheader"

SPECIALS	[()<>@,;:\\".\[\]]
ATOMCHAR	([a-zA-Z0-9`~!#$%^&*\-_=+{}|?])
ATOM		({ATOMCHAR}{1,80})
INUM	(([0-9])|([0-9][0-9])|(1[0-9][0-9])|(2[0-4][0-9])|(25[0-5]))
HEX		([0-9a-f])
XPC	[ !#$%&'()*+,\-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ\[\\\]^_`abcdefghijklmnopqrstuvwxyz{|}~]
PC	[ !#$%&'()*+,\-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ\[\\\]^_`abcdefghijklmnopqrstuvwxyz{|}~"]
ALNUM	[a-zA-Z0-9]
TLD		(AC|AD|AE|AERO|AF|AG|AI|AL|AM|AN|AO|AQ|AR|ARPA|AS|ASIA|AT|AU|AW|AX|AZ|BA|BB|BD|BE|BF|BG|BH|BI|BIZ|BJ|BL|BM|BN|BO|BR|BS|BT|BV|BW|BY|BZ|CA|CAT|CC|CD|CF|CG|CH|CI|CK|CL|CM|CN|CO|COM|COOP|CR|CU|CV|CX|CY|CZ|DE|DJ|DK|DM|DO|DZ|EC|EDU|EE|EG|EH|ER|ES|ET|EU|FI|FJ|FK|FM|FO|FR|GA|GB|GD|GE|GF|GG|GH|GI|GL|GM|GN|GOV|GP|GQ|GR|GS|GT|GU|GW|GY|HK|HM|HN|HR|HT|HU|ID|IE|IL|IM|IN|INFO|INT|IO|IQ|IR|IS|IT|JE|JM|JO|JOBS|JP|KE|KG|KH|KI|KM|KN|KP|KR|KW|KY|KZ|LA|LB|LC|LI|LK|LR|LS|LT|LU|LV|LY|MA|MC|MD|ME|MF|MG|MH|MIL|MK|ML|MM|MN|MO|MOBI|MP|MQ|MR|MS|MT|MU|MUSEUM|MV|MW|MX|MY|MZ|NA|NAME|NC|NE|NET|NF|NG|NI|NL|NO|NP|NR|NU|NZ|OM|ORG|PA|PE|PF|PG|PH|PK|PL|PM|PN|PR|PRO|PS|PT|PW|PY|QA|RE|RO|RS|RU|RW|SA|SB|SC|SD|SE|SG|SH|SI|SJ|SK|SL|SM|SN|SO|SR|ST|SU|SV|SY|SZ|TC|TD|TEL|TF|TG|TH|TJ|TK|TL|TM|TN|TO|TP|TR|TRAVEL|TT|TV|TW|TZ|UA|UG|UK|UM|US|UY|UZ|VA|VC|VE|VG|VI|VN|VU|WF|WS|YE|YT|YU|ZA|ZM|ZW)


DOMAINREF	{ATOM}
SUBDOMAIN	{DOMAINREF}
DOMAIN		({SUBDOMAIN}[.])*{SUBDOMAIN}
QTEXT		[a-zA-Z0-9`~!@#$%^&*()_\-+=\[\]{}\\|;:',.<>/?]
QUOTEDSTRING	["]{QTEXT}*["]
WORD		({ATOM})|({QUOTEDSTRING})
LOCALPART	{WORD}([.]{WORD})*
ADDRSPEC	{LOCALPART}@{DOMAIN}
MAILBOX		{ADDRSPEC}

YEAR		((19[6-9][0-9])|(200[0-9]))
DAYOFWEEK	(Mon|Tue|Wed|Thu|Fri|Sat|Sun)
MONTH		(Jan|Feb|Mar|Apr|May|Jun|Jul|Aug|Sep|Oct|Nov|Dec)
ABBREV		(UT|GMT|EST|EDT|CST|CDT|MST|MDT|PST|PDT|Z|A|M|N|Y)


U_TLD1		(A\0C\0|A\0D\0|A\0E\0|A\0E\0R\0O\0|A\0F\0|A\0G\0|A\0I\0|A\0L\0|A\0M\0|A\0N\0|A\0O\0|A\0Q\0|A\0R\0|A\0R\0P\0A\0|A\0S\0|A\0S\0I\0A\0|A\0T\0|A\0U\0|A\0W\0|A\0X\0|A\0Z\0|B\0A\0|B\0B\0|B\0D\0|B\0E\0|B\0F\0|B\0G\0|B\0H\0|B\0I\0|B\0I\0Z\0|B\0J\0|B\0L\0|B\0M\0|B\0N\0|B\0O\0|B\0R\0|B\0S\0|B\0T\0|B\0V\0|B\0W\0|B\0Y\0|B\0Z\0|C\0A\0|C\0A\0T\0|C\0C\0|C\0D\0|C\0F\0|C\0G\0|C\0H\0|C\0I\0|C\0K\0|C\0L\0|C\0M\0|C\0N\0|C\0O\0|C\0O\0M\0|C\0O\0O\0P\0|C\0R\0|C\0U\0|C\0V\0|C\0X\0|C\0Y\0|C\0Z\0)
U_TLD2		(D\0E\0|D\0J\0|D\0K\0|D\0M\0|D\0O\0|D\0Z\0|E\0C\0|E\0D\0U\0|E\0E\0|E\0G\0|E\0H\0|E\0R\0|E\0S\0|E\0T\0|E\0U\0|F\0I\0|F\0J\0|F\0K\0|F\0M\0|F\0O\0|F\0R\0|G\0A\0|G\0B\0|G\0D\0|G\0E\0|G\0F\0|G\0G\0|G\0H\0|G\0I\0|G\0L\0|G\0M\0|G\0N\0|G\0O\0V\0|G\0P\0|G\0Q\0|G\0R\0|G\0S\0|G\0T\0|G\0U\0|G\0W\0|G\0Y\0|H\0K\0|H\0M\0|H\0N\0|H\0R\0|H\0T\0|H\0U\0|I\0D\0|I\0E\0|I\0L\0|I\0M\0|I\0N\0|I\0N\0F\0O\0|I\0N\0T\0|I\0O\0|I\0Q\0|I\0R\0|I\0S\0|I\0T\0|J\0E\0|J\0M\0|J\0O\0|J\0O\0B\0S\0|J\0P\0|K\0E\0|K\0G\0|K\0H\0|K\0I\0|K\0M\0|K\0N\0|K\0P\0|K\0R\0|K\0W\0|K\0Y\0|K\0Z\0)
U_TLD3		(L\0A\0|L\0B\0|L\0C\0|L\0I\0|L\0K\0|L\0R\0|L\0S\0|L\0T\0|L\0U\0|L\0V\0|L\0Y\0|M\0A\0|M\0C\0|M\0D\0|M\0E\0|M\0F\0|M\0G\0|M\0H\0|M\0I\0L\0|M\0K\0|M\0L\0|M\0M\0|M\0N\0|M\0O\0|M\0O\0B\0I\0|M\0P\0|M\0Q\0|M\0R\0|M\0S\0|M\0T\0|M\0U\0|M\0U\0S\0E\0U\0M\0|M\0V\0|M\0W\0|M\0X\0|M\0Y\0|M\0Z\0|N\0A\0|N\0A\0M\0E\0|N\0C\0|N\0E\0|N\0E\0T\0|N\0F\0|N\0G\0|N\0I\0|N\0L\0|N\0O\0|N\0P\0|N\0R\0|N\0U\0|N\0Z\0|O\0M\0|O\0R\0G\0|P\0A\0|P\0E\0|P\0F\0|P\0G\0|P\0H\0|P\0K\0|P\0L\0|P\0M\0|P\0N\0|P\0R\0|P\0R\0O\0|P\0S\0|P\0T\0|P\0W\0|P\0Y\0)
U_TLD4		(Q\0A\0|R\0E\0|R\0O\0|R\0S\0|R\0U\0|R\0W\0|S\0A\0|S\0B\0|S\0C\0|S\0D\0|S\0E\0|S\0G\0|S\0H\0|S\0I\0|S\0J\0|S\0K\0|S\0L\0|S\0M\0|S\0N\0|S\0O\0|S\0R\0|S\0T\0|S\0U\0|S\0V\0|S\0Y\0|S\0Z\0|T\0C\0|T\0D\0|T\0E\0L\0|T\0F\0|T\0G\0|T\0H\0|T\0J\0|T\0K\0|T\0L\0|T\0M\0|T\0N\0|T\0O\0|T\0P\0|T\0R\0|T\0R\0A\0V\0E\0L\0|T\0T\0|T\0V\0|T\0W\0|T\0Z\0|U\0A\0|U\0G\0|U\0K\0|U\0M\0|U\0S\0|U\0Y\0|U\0Z\0|V\0A\0|V\0C\0|V\0E\0|V\0G\0|V\0I\0|V\0N\0|V\0U\0|W\0F\0|W\0S\0|Y\0E\0|Y\0T\0|Y\0U\0|Z\0A\0|Z\0M\0|Z\0W\0)

 /*
  * RFC 2616, Page 12
  */
 /* Account for over-zealously translated line breaks */
 /* HTTP_LWS - Linear White Space (new line and a whitespace character) */
HTTP_CRLF	(\x0D\x0A|\x0A)
HTTP_LWS	{HTTP_CRLF}[ \t]

 /*
  * RFC 2616, Page 13
  * A TOKEN is any CHAR (0-127) except a CTL or a SEPARATOR.
  * SLG: For this parser, we limit tokens to a max of 128 bytes. 
  */
HTTP_TOKEN	([\x00-\x7F]{-}[\x00-\x1F\x7F]{-}[()<>@,;\:\\"/\[\]?=\{\}\x09\x20]){1,128}

 /*
  * Keeping it simple: no HTTP_CTEXT, HTTP_QUOTED_PAIR, or keeping count
  * of parentheses. The distinguishing part of COMMENTs is they are
  * allowed to have line breaks, if followed by whitespace.
  *
  * TODO Might still need to account for RFC 2407.
  */
HTTP_COMMENT	({PC}|{HTTP_LWS}|[\t])

 /*
  * RFC 2616, Section 3.8, page 20
  * HTTP Product and Version
  */
HTTP_PRODUCT	{HTTP_TOKEN}(\/{HTTP_TOKEN})?
HTTP_VERSION	HTTP\/[0-9]\.[0-9]

 /*
  * RFC 2616, Section 5.1.1, Page 24
  * Request-Line and method. Specifying a URI or HTTP_TOKENs here has
  * not yet worked, so simply use non-dquote characters.
  *
  * SLG: XPC is now a maximum of 128 characters
  */
HTTP_METHOD	(OPTIONS|GET|HEAD|POST|PUT|DELETE|TRACE|CONNECT|{HTTP_TOKEN})
HTTP_REQUEST_LINE	{HTTP_METHOD}[ ]({XPC}{1,128}){HTTP_VERSION}

 /*
  * RFC 2616, Section 6.1
  * SLG: reasons are now limited to 0-40 characters (previously it was '*')
  */
HTTP_REASON_PHRASE	{PC}{0,40}
HTTP_STATUS_CODE	[0-9][0-9][0-9]
HTTP_STATUS_LINE	{HTTP_VERSION}[ ]{HTTP_STATUS_CODE}[ ]{HTTP_REASON_PHRASE}

%%

{HTTP_REQUEST_LINE} {
    httpheader_recorder->write_buf(sbuf,pos,yyleng);
    pos += yyleng;
}

{HTTP_STATUS_LINE} {
    httpheader_recorder->write_buf(sbuf,pos,yyleng);
    pos += yyleng;
}

 /*
  * RFC 2616, Sections 14.38 and 14.43
  * These fields are allowed multi-line values (comments).
  *
  * For some reason, specifying the field value as:
  *   ({XPC}|{HTTP_LWS})+
  * causes the NFA rule set to explode to >32000 rules, making flex refuse to compile.
  */
(Server|User-Agent):[ \t]?({PC}{1,80}) {
    httpheader_recorder->write_buf(sbuf,pos,yyleng);
    pos += yyleng;
}

 /*
  * RFC 2616, Section 14.23
  */
Host:[ \t]?([a-zA-Z0-9._:]{1,256}) {   
    httpheader_recorder->write_buf(sbuf,pos,yyleng);
    pos += yyleng; 
}

 /*
  * These headers have a general set of characters allowed in their field value, including double-quote.
  *
  * Keep-Alive is defined in RFC 2068, Section 19.7.1.1. Allowable tokens seem to include doublequote,
  * per "value" definition in RFC 2068, Section 3.7.
  *
  * Authorization, Proxy-Authenticate, Proxy-Authorization and WWW-Authenticate are defined in RFC 2617, not yet reviewed.  
  * Assuming PC character set allowed for now.
  *
  * Content-Location, Location and Referer (RFC 2616, Sections 14.14, 14.30 and 14.36) have a URI as the field value.
  * SLG: Limited to 80 characters
  */
(Accept|Accept-Ranges|Authorization|Cache-Control|Content-Location|Etag|Expect|Keep-Alive|If-Match|If-None-Match|If-Range|Pragma|Proxy-Authenticate|Proxy-Authorization|Referer|TE|Transfer-Encoding|Warning|WWW-Authenticate):[ \t]?({PC}){1,80} {
    httpheader_recorder->write_buf(sbuf,pos,yyleng);
    pos += yyleng;
}


 /*
  * These headers have a general set of characters allowed in their field value, excluding double-quote.
  *
  * Date and If-Modified-Since reference RFCs 1123 and 850 (RFC 2616 Section 3.3.1), not yet reviewed.
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
(Accept-Charset|Accept-Encoding|Accept-Language|Age|Allow|Connection|Content-Encoding|Content-Language|Content-MD5|Content-Range|Content-Type|Cookie|Date|From|If-Modified-Since|If-Unmodified-Since|Last-Modified|Range|Retry-After|Set-Cookie|Trailer|Upgrade|Vary):[ \t]?({XPC}){1,80} {
    httpheader_recorder->write_buf(sbuf,pos,yyleng);
    pos += yyleng;
}

Via:[ \t]?{HTTP_COMMENT}{1,256}	{
    httpheader_recorder->write_buf(sbuf,pos,yyleng);
    pos += yyleng;
}

 /*
  * RFC 2616, Sections 14.13 and 14.31
  */
(Content-Length|Max-Forwards):[ \t]?[0-9]{1,12} {
    httpheader_recorder->write_buf(sbuf,pos,yyleng);
    pos += yyleng;
}

.|\n { 
     /**
      * The no-match rule.
      * If we are beyond the end of the margin, call it quits.
      */
     /* putchar(yytext[0]); */ /* Uncomment for debugging */
     pos++; 
     if(pos>=sbuf.pagesize) eof=true;
}
%%

extern "C"
void scan_httpheader(const class scanner_params &sp,const recursion_control_block &rcb)
{
    assert(sp.version==scanner_params::CURRENT_SP_VERSION);      
    if(sp.phase==scanner_params::startup){
        assert(sp.info->si_version==scanner_info::CURRENT_SI_VERSION);
        sp.info->name		= "httpheader";
        sp.info->author         = "Alex Nelson";
        sp.info->description    = "Searches for HTTP header fields";
        sp.info->scanner_version= "1.0";
        sp.info->flags = scanner_info::SCANNER_DISABLED;
        sp.info->feature_names.insert("httpheader");
        return;
    }
    if(sp.phase==scanner_params::scan){
        MyHTTPHeaderFlexLexer  lexer(sp.sbuf);
    	lexer.httpheader_recorder  = sp.fs.get_name("httpheader");
	while(lexer.yylex() != 0) {
    	}
    }
}

