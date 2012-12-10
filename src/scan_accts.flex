%{

#include "config.h"
#include "bulk_extractor_i.h"
#include "histogram.h"

/*
 * http://flex.sourceforge.net/manual/Cxx.html
 *
 * Credit card scanner (and then some).
 * For references, see:
 * http://en.wikipedia.org/wiki/Bank_card_number
 * http://en.wikipedia.org/wiki/List_of_Bank_Identification_Numbers
 */

size_t min_phone_digits=6;

#include "bulk_extractor_i.h"
#include "scan_ccns2.h"

#include "sbuf_flex_scanner.h"
class accts_scanner : public sbuf_scanner {
public:
	accts_scanner(const scanner_params &sp):
	  sbuf_scanner(&sp.sbuf),
	  ccn_recorder(),ccn_track2(),telephone_recorder(),alert_recorder(){

  	    ccn_recorder       = sp.fs.get_name("ccn");
            telephone_recorder = sp.fs.get_name("telephone");
    	    ccn_track2         = sp.fs.get_name("ccn_track2");
    	    alert_recorder     = sp.fs.get_name(feature_recorder_set::ALERT_RECORDER_NAME);
	}

	class feature_recorder *ccn_recorder;
	class feature_recorder *ccn_track2;
	class feature_recorder *telephone_recorder;
	class feature_recorder *alert_recorder;
};
#define YY_EXTRA_TYPE accts_scanner *             /* holds our class pointer */
YY_EXTRA_TYPE yyaccts_get_extra (yyscan_t yyscanner );    /* redundent declaration */
inline class accts_scanner *get_extra(yyscan_t yyscanner) {return yyaccts_get_extra(yyscanner);}

static bool has_min_digits(const char *buf)
{
   unsigned int digit_count = 0;
   for(const char *cc=buf;*cc;cc++){
      if(isdigit(*cc)) digit_count+=1;
   }
   return digit_count>=min_phone_digits;
}

static string utf16to8(const wstring &s){
string utf8_line;
	try {
	    utf8::utf16to8(s.begin(),s.end(),back_inserter(utf8_line));
	} catch(utf8::invalid_utf16){
	    /* Exception thrown: bad UTF16 encoding */
	    utf8_line = "";
	}
	return utf8_line;
    }
%}

%option noyywrap
%option 8bit
%option batch
%option case-insensitive
%option pointer
%option noyymore
%option prefix="yyaccts_"

END	([^0-9e\.]|(\.[^0-9]))
BLOCK    [0-9]{4}
DELIM	 ([- ])
DB	({BLOCK}{DELIM})
SDB	([45][0-9][0-9][0-9]{DELIM})
TDEL	([ /.-])

PHONETEXT ([^a-z](tel[.ephon]*|(fax)|(facsimile)|DSN|telex|TTD|mobile|cell)):?

YEAR		((19[0-9][0-9])|(20[01][0-9]))
DAYOFWEEK	(Mon|Tue|Wed|Thu|Fri|Sat|Sun)
DAY		([0-9]|[0-2][0-9]|30|31)
MONTH		(Jan(uary)?|Feb(ruary)?|Mar(ch)?|Apr(il)?|May|Jun(e)?|Jul(y)?|Aug(ust)?|Sep(tember)?|Oct(ober)?|Nov(ember)?|Dec(ember)?|0?1|0?2|0?3|0?4|0?5|0?6|0?7|0?8|0?9|10|11|12)
SYEAR		([0-9][0-9])
SMONTH		([0-1][0-2])

DATEA	({YEAR}-{MONTH}-{DAY})
DATEB	({YEAR}[/]{MONTH}[/]{DAY})
DATEC	({DAY}[ ]{MONTH}[ ]{YEAR})
DATED	({MONTH}[ ]{DAY}[, ]+{YEAR})

DATEFORMAT	({DATEA}|{DATEB}|{DATEC}|{DATED})

%%

[^0-9a-z]{DB}{DB}{DB}{DB}{DB} {
    /* #### #### #### #### #### #### is definately not a CCN. */
    /* REGEX1 */
    accts_scanner &s = *yyaccts_get_extra(yyscanner);
    s.pos += yyleng;
}

[^0-9a-z]{SDB}{DB}{DB}{BLOCK}/{END} {
    /* REGEX2 */
    /* #### #### #### #### --- most credit card numbers*/
    /* don't include the non-numeric character in the hand-off */
    accts_scanner &s = *yyaccts_get_extra(yyscanner);
    if(validate_ccn(yytext+1,yyleng-1)){
        s.ccn_recorder->write_buf(SBUF,s.pos+1,yyleng-1);
    }	
    s.pos += yyleng;
}

[^0-9a-z\.][3][0-9][0-9][0-9]{DELIM}[0-9]{6}{DELIM}[0-9]{5}/{END} {
    /* 3### ###### ######### --- 15 digits beginning with 3 and funny space. */
    /* REGEX3 */
    /* Must be american express... */ 
    accts_scanner &s = *yyaccts_get_extra(yyscanner);
    if(validate_ccn(yytext+1,yyleng-1)){
        s.ccn_recorder->write_buf(SBUF,s.pos+1,yyleng-1);
    }	
    s.pos += yyleng;
}

[^0-9a-z\.][3]([0-9]{14})/{END} {
    /* 3### ###### ######### --- 15 digits beginning with 3 and funny space. */
    /* REGEX4 */
    /* Must be american express... */ 
    accts_scanner &s = *yyaccts_get_extra(yyscanner);
    if(validate_ccn(yytext+1,yyleng-1)){
        s.ccn_recorder->write_buf(SBUF,s.pos+1,yyleng-1);
    }	
    s.pos += yyleng;
}

[^0-9a-z\.][4-6]([0-9]{15,18})/{END} {
    /* REGEX5 */
    /* ###############  13-19 numbers as a block beginning with a 4 or 5
     * followed by something that is not a digit.
     * Yes, CCNs can now be up to 19 digits long. 
     * http://www.creditcards.com/credit-card-news/credit-card-appearance-1268.php
     */
    accts_scanner &s = *yyaccts_get_extra(yyscanner);
    if(validate_ccn(yytext+1,yyleng-1)){
        s.ccn_recorder->write_buf(SBUF,s.pos+1,yyleng-1);
    }	
    s.pos += yyleng;
}

[^0-9a-z][4-6]([0-9]{15,18})\={SYEAR}{SMONTH}101[0-9]{13} {
    /* ;###############=YYMM101#+? --- track2 credit card data */
    /* {SYEAR}{SMONTH} */
    /* ;CCN=05061010000000000738? */
    /* REGEX6 */
    accts_scanner &s = *yyaccts_get_extra(yyscanner);
    if(validate_ccn(yytext+1,16)){  /* validate the first 16 digits */
    	s.ccn_track2->write_buf(SBUF,s.pos+1,yyleng-1);
    }
    s.pos += yyleng;
}

[^0-9a-z][0-9][0-9][0-9]{TDEL}[0-9][0-9][0-9]{TDEL}[0-9][0-9][0-9][0-9]/{END} {
    /* REGEX7 */
    /* US phone numbers without area code in parens */
    /* New addition: If proceeded by " ####? ####? " 
     * then do not consider this a phone number. We see a lot of that stuff in
     * PDF files.
     */
    accts_scanner &s = *yyaccts_get_extra(yyscanner);
    if(validate_phone(SBUF,s.pos+1,yyleng-1)){
       s.telephone_recorder->write_buf(SBUF,s.pos+1,yyleng-1);
    }
    s.pos += yyleng;
}

[^0-9a-z]\([0-9][0-9][0-9]\){TDEL}?[0-9][0-9][0-9]{TDEL}[0-9][0-9][0-9][0-9]/{END} {
    /* REGEX8 */
    /* US phone number with parens, like (215) 555-1212 */
    accts_scanner &s = *yyaccts_get_extra(yyscanner);
    s.telephone_recorder->write_buf(SBUF,s.pos+1,yyleng-1);
    s.pos += yyleng;
}

[^0-9a-z]\+[0-9]{1,3}(({TDEL}[0-9][0-9][0-9]?){2,6})[0-9]{2,4}/{END} {
    /* REGEX9 */
    /* Generalized international phone numbers */
    accts_scanner &s = *yyaccts_get_extra(yyscanner);
    if(has_min_digits(yytext)){
        if(validate_phone(SBUF,s.pos+1,yyleng-1)){
            s.telephone_recorder->write_buf(SBUF,s.pos+1,yyleng-1);
        }
    }
    s.pos += yyleng;
}

{PHONETEXT}[0-9/ .+]{7,18} {
    /* REGEX10 */
    /* Generalized number with prefix */
    accts_scanner &s = *yyaccts_get_extra(yyscanner);
    s.telephone_recorder->write_buf(SBUF,s.pos+1,yyleng);
    s.pos += yyleng;
}

{PHONETEXT}[0-9 +]+[ ]?[(][0-9]{2,4}[)][ ]?[\-0-9]{4,8} {
    /* REGEX11 */
    /* Generalized number with city code and prefix */
    accts_scanner &s = *yyaccts_get_extra(yyscanner);
    s.telephone_recorder->write_buf(SBUF,s.pos+1,yyleng-1);
    s.pos += yyleng;
}

[^0-9]([0-9]{6}-){7}([0-9]{6})/[\r\n] {
    accts_scanner &s = *yyaccts_get_extra(yyscanner);
    s.alert_recorder->write(SBUF.pos0+s.pos,yytext+1,"Possible BitLocker Recovery Key (ASCII).");
    s.pos += yyleng;
}

\0((([0-9]\0){6}-\0){7}(([0-9]\0){6}))/[^0-9] {
    /* Make a wstring from yytext+1 to yyleng */
    accts_scanner &s = *yyaccts_get_extra(yyscanner);
    wstring stmp;
    for(size_t i=1;i+1<(size_t)yyleng;i+=2){
       stmp.push_back(yytext[i] | (yytext[i+1]<<8));
    }
    s.alert_recorder->write(SBUF.pos0+s.pos,utf16to8(stmp),"Possible BitLocker Recovery Key (UTF-16)");
    s.pos += yyleng;
}


fedex[^a-z]+[0-9][0-9][0-9][0-9][- ]?[0-9][0-9][0-9][0-9][- ]?[0-9]/{END}	{
    /* REGEX12 */
    /* Generalized international phone numbers */
    accts_scanner &s = *yyaccts_get_extra(yyscanner);
    s.ccn_recorder->write_buf(SBUF,s.pos,yyleng);
    s.pos += yyleng;
}

ssn:?[ \t]+[0-9][0-9][0-9]-?[0-9][0-9]-?[0-9][0-9][0-9][0-9]/{END}	{
    /* REGEX13 */
    accts_scanner &s = *yyaccts_get_extra(yyscanner);
    s.ccn_recorder->write_buf(SBUF,s.pos,yyleng);
    s.pos += yyleng;
}

dob:?[ \t]+{DATEFORMAT}	{
    /* REGEX14 */
    accts_scanner &s = *yyaccts_get_extra(yyscanner);
    s.ccn_recorder->write_buf(SBUF,s.pos,yyleng);
    s.pos += yyleng;
}

box[ ]?[\[][0-9 -]{0,40}[\]] {
/* Common box arrays found in PDF files
 * With more testing this can and will still be tweaked
 */
    accts_scanner &s = *yyaccts_get_extra(yyscanner);
    s.pos += yyleng;
}

[\[][ ]?[0-9.-]{1,12}[ ][0-9.-]{1,12}[ ][0-9.-]{1,12}[ ][0-9.-]{1,12}[ ]?[\]] {
/* Common rectangles found in PDF files
 *  With more testing this can and will still be tweaked
 */
    accts_scanner &s = *yyaccts_get_extra(yyscanner);
    s.pos += yyleng;
}

.|\n { 
     /**
      * The no-match rule.
      * If we are beyond the end of the margin, call it quits.
      */
     accts_scanner &s = *yyaccts_get_extra(yyscanner);
     /* putchar(yytext[0]); */ /* Uncomment for debugging */
     s.pos++; 
}

%%

extern "C"
void scan_accts(const class scanner_params &sp,const recursion_control_block &rcb)
{
    assert(sp.sp_version==scanner_params::CURRENT_SP_VERSION);      
    if(sp.phase==scanner_params::startup){
        assert(sp.info->si_version==scanner_info::CURRENT_SI_VERSION);
	sp.info->name  = "accts";
	sp.info->author		= "Simson L. Garfinkel";
	sp.info->description	= "scans for CCNs, track 2, and phone #s";
	sp.info->scanner_version= "1.0";
        sp.info->feature_names.insert("ccn");
        sp.info->feature_names.insert("ccn_track2");
        sp.info->feature_names.insert("telephone");
        sp.info->feature_names.insert(feature_recorder_set::ALERT_RECORDER_NAME);
	sp.info->histogram_defs.insert(histogram_def("ccn","","histogram"));
	sp.info->histogram_defs.insert(histogram_def("ccn_track2","","histogram"));
	sp.info->histogram_defs.insert(histogram_def("telephone","","histogram",HistogramMaker::FLAG_NUMERIC));
	return;
    }
    if(sp.phase==scanner_params::scan){
        accts_scanner lexer(sp);
	yyscan_t scanner;
        yyaccts_lex_init(&scanner);
	yyaccts_set_extra(&lexer,scanner);
	yyaccts_lex(scanner);
        yyaccts_lex_destroy(scanner);
	(void)yyunput;			// avoids defined but not used
    }
}
