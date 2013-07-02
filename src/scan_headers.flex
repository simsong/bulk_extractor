%{
/*
 *  http://flex.sourceforge.net/manual/Cxx.html
 *
 * Don't use this one. It was developed for Alex Nelson's research project.
 * It is supposed to find all RFC 822 headers, but it over-reports.
 */

#include "config.h"
#include "be13_api/bulk_extractor_i.h"

#include <stdlib.h>
#include <string.h>

#undef 	ECHO
#define	ECHO {}

#define YY_SKIP_YYWRAP
#define YY_DECL int MyScanFlexLexer::yylex()

class MyScanFlexLexer : public emailFlexLexer {
      public:
      MyScanFlexLexer(const sbuf_t &sbuf_):sbuf(sbuf_){
	  this->pos		= 0;
          this->point		= 0;
	  this->eof             = false;
      }
      const sbuf_t &sbuf;
      size_t pos;
      size_t point;
      bool eof;
      class feature_recorder *email_recorder;
      class feature_recorder *rfc822_recorder;
      class feature_recorder *domain_recorder;
      class feature_recorder *url_recorder;
      virtual int LexerInput(char *buf,int max_size_){  /* signature can't be changed */
      	      if(eof) return 0;
      	      u_int max_size = max_size_ > 0 ? max_size_ : 0;
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

int emailFlexLexer::yylex(){return 0;}


/* Address some common false positives in email scanner */
bool validate_email(const char *email)
{
    if(strstr(email,"..")) return false;
    return true;
}




%}

%option c++
%option noyywrap
%option 8bit
%option batch
%option case-insensitive
%option pointer
%option noyymore
%option prefix="email"

SPECIALS	[()<>@,;:\\".\[\]]
ATOMCHAR	([a-zA-Z0-9`~!#$%^&*\-_=+{}|?])
ATOM		({ATOMCHAR}{1,80})
INUM	(([0-9])|([0-9][0-9])|(1[0-9][0-9])|(2[0-4][0-9])|(25[0-5]))
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

EMAIL		{ALNUM}[a-zA-Z0-9._%\-+]{1,128}{ALNUM}@{ALNUM}[a-zA-Z0-9._%\-]{1,128}\.{TLD}
YEAR		((19[6-9][0-9])|(20[0-1][0-9]))
DAYOFWEEK	(Mon|Tue|Wed|Thu|Fri|Sat|Sun)
MONTH		(Jan|Feb|Mar|Apr|May|Jun|Jul|Aug|Sep|Oct|Nov|Dec)
ABBREV		(UT|GMT|EST|EDT|CST|CDT|MST|MDT|PST|PDT|Z|A|M|N|Y)


U_TLD1		(A\0C\0|A\0D\0|A\0E\0|A\0E\0R\0O\0|A\0F\0|A\0G\0|A\0I\0|A\0L\0|A\0M\0|A\0N\0|A\0O\0|A\0Q\0|A\0R\0|A\0R\0P\0A\0|A\0S\0|A\0S\0I\0A\0|A\0T\0|A\0U\0|A\0W\0|A\0X\0|A\0Z\0|B\0A\0|B\0B\0|B\0D\0|B\0E\0|B\0F\0|B\0G\0|B\0H\0|B\0I\0|B\0I\0Z\0|B\0J\0|B\0L\0|B\0M\0|B\0N\0|B\0O\0|B\0R\0|B\0S\0|B\0T\0|B\0V\0|B\0W\0|B\0Y\0|B\0Z\0|C\0A\0|C\0A\0T\0|C\0C\0|C\0D\0|C\0F\0|C\0G\0|C\0H\0|C\0I\0|C\0K\0|C\0L\0|C\0M\0|C\0N\0|C\0O\0|C\0O\0M\0|C\0O\0O\0P\0|C\0R\0|C\0U\0|C\0V\0|C\0X\0|C\0Y\0|C\0Z\0)
U_TLD2		(D\0E\0|D\0J\0|D\0K\0|D\0M\0|D\0O\0|D\0Z\0|E\0C\0|E\0D\0U\0|E\0E\0|E\0G\0|E\0H\0|E\0R\0|E\0S\0|E\0T\0|E\0U\0|F\0I\0|F\0J\0|F\0K\0|F\0M\0|F\0O\0|F\0R\0|G\0A\0|G\0B\0|G\0D\0|G\0E\0|G\0F\0|G\0G\0|G\0H\0|G\0I\0|G\0L\0|G\0M\0|G\0N\0|G\0O\0V\0|G\0P\0|G\0Q\0|G\0R\0|G\0S\0|G\0T\0|G\0U\0|G\0W\0|G\0Y\0|H\0K\0|H\0M\0|H\0N\0|H\0R\0|H\0T\0|H\0U\0|I\0D\0|I\0E\0|I\0L\0|I\0M\0|I\0N\0|I\0N\0F\0O\0|I\0N\0T\0|I\0O\0|I\0Q\0|I\0R\0|I\0S\0|I\0T\0|J\0E\0|J\0M\0|J\0O\0|J\0O\0B\0S\0|J\0P\0|K\0E\0|K\0G\0|K\0H\0|K\0I\0|K\0M\0|K\0N\0|K\0P\0|K\0R\0|K\0W\0|K\0Y\0|K\0Z\0)
U_TLD3		(L\0A\0|L\0B\0|L\0C\0|L\0I\0|L\0K\0|L\0R\0|L\0S\0|L\0T\0|L\0U\0|L\0V\0|L\0Y\0|M\0A\0|M\0C\0|M\0D\0|M\0E\0|M\0F\0|M\0G\0|M\0H\0|M\0I\0L\0|M\0K\0|M\0L\0|M\0M\0|M\0N\0|M\0O\0|M\0O\0B\0I\0|M\0P\0|M\0Q\0|M\0R\0|M\0S\0|M\0T\0|M\0U\0|M\0U\0S\0E\0U\0M\0|M\0V\0|M\0W\0|M\0X\0|M\0Y\0|M\0Z\0|N\0A\0|N\0A\0M\0E\0|N\0C\0|N\0E\0|N\0E\0T\0|N\0F\0|N\0G\0|N\0I\0|N\0L\0|N\0O\0|N\0P\0|N\0R\0|N\0U\0|N\0Z\0|O\0M\0|O\0R\0G\0|P\0A\0|P\0E\0|P\0F\0|P\0G\0|P\0H\0|P\0K\0|P\0L\0|P\0M\0|P\0N\0|P\0R\0|P\0R\0O\0|P\0S\0|P\0T\0|P\0W\0|P\0Y\0)
U_TLD4		(Q\0A\0|R\0E\0|R\0O\0|R\0S\0|R\0U\0|R\0W\0|S\0A\0|S\0B\0|S\0C\0|S\0D\0|S\0E\0|S\0G\0|S\0H\0|S\0I\0|S\0J\0|S\0K\0|S\0L\0|S\0M\0|S\0N\0|S\0O\0|S\0R\0|S\0T\0|S\0U\0|S\0V\0|S\0Y\0|S\0Z\0|T\0C\0|T\0D\0|T\0E\0L\0|T\0F\0|T\0G\0|T\0H\0|T\0J\0|T\0K\0|T\0L\0|T\0M\0|T\0N\0|T\0O\0|T\0P\0|T\0R\0|T\0R\0A\0V\0E\0L\0|T\0T\0|T\0V\0|T\0W\0|T\0Z\0|U\0A\0|U\0G\0|U\0K\0|U\0M\0|U\0S\0|U\0Y\0|U\0Z\0|V\0A\0|V\0C\0|V\0E\0|V\0G\0|V\0I\0|V\0N\0|V\0U\0|W\0F\0|W\0S\0|Y\0E\0|Y\0T\0|Y\0U\0|Z\0A\0|Z\0M\0|Z\0W\0)

%%

"{XPC}{1,80}"[ ]+[<]{MAILBOX}[>] {
    printf(">>> SCAN_HEADERS 1: %s\n",yytext);
    pos += yyleng;
}

[ ]{XPC}{1,80}[ ]+[<]{MAILBOX}[>] {
    printf(">>> SCAN_HEADERS 2: %s\n",yytext);
    pos += yyleng;
}

[ ]{MAILBOX}[ ]+\((ATOMCHAR|[ ]){1,80}\) {
    printf(">>> SCAN_HEADERS 4: %s\n",yytext);
    pos += yyleng;
}

[ ]{XPC}{1,80}[ ]+\[mailto:{MAILBOX}\] {
    printf(">>> SCAN_HEADERS 3: %s\n",yytext);
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

void scan_headers(const class scanner_params &sp,const recursion_control_block &rcb)
{
    if(sp.feature_names){
        sp.feature_names->insert("header");
    }
    if(sp.fs==0) return;	

    MyScanFlexLexer  lexer(sp.sbuf);
    lexer.header_recorder = sp.fs->get_name("header");

    while(lexer.yylex() != 0) {
    }
}

