%{

#include "config.h"
#include "be13_api/bulk_extractor_i.h"
#include "histogram.h"
#include "scan_ccns2.h"
#include "sbuf_flex_scanner.h"


/*
 * http://flex.sourceforge.net/manual/Cxx.html
 *
 * Credit card scanner (and then some).
 * For references, see:
 * http://en.wikipedia.org/wiki/Bank_card_number
 * http://en.wikipedia.org/wiki/List_of_Bank_Identification_Numbers
 */

size_t min_phone_digits=7;
static int ssn_mode=0;

class accts_scanner : public sbuf_scanner {
public:
	accts_scanner(const scanner_params &sp):
	  sbuf_scanner(&sp.sbuf),
	  ccn_recorder(sp.fs.get_name("ccn")),
          pii_recorder(sp.fs.get_name("pii")),
          sin_recorder(sp.fs.get_name("sin")),
          ccn_track2(sp.fs.get_name("ccn_track2")),
          telephone_recorder(sp.fs.get_name("telephone")),
          alert_recorder(sp.fs.get_alert_recorder()){
	}

	class feature_recorder *ccn_recorder;
    class feature_recorder *pii_recorder;
    class feature_recorder *sin_recorder;
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

/*
 * http://www.reddnet.net/regular-expression-for-validating-a-social-security-number-ssn-issued-after-june-25-2011/
 * http://www.ssa.gov/employer/randomization.html
 */
static bool valid_ssn(const std::string &ssn)
{
   std::string nums;
   /* Extract the digits */
   for(int i=0;i<ssn.size();i++){
      nums.push_back(ssn.at(i));
   }
   /* Apply the new validation rules */
   if(nums.size()!=9) return false;
   if(nums.substr(0,3)=="000") return false;
   if(nums.substr(3,2)=="00") return false;
   if(nums.substr(5,4)=="0000") return false;
   if(nums.substr(0,3)=="666") return false;
   if(nums[0]=='9') return false;
   return true;
}

static std::string utf16to8(const std::wstring &s){
std::string utf8_line;
	try {
	    utf8::utf16to8(s.begin(),s.end(),back_inserter(utf8_line));
	} catch(const utf8::invalid_utf16 &){
	    /* Exception thrown: bad UTF16 encoding */
	    utf8_line = "";
	}
	return utf8_line;
}

bool is_fbid(const sbuf_t &sbuf, size_t loc) {
     if (loc>5 && sbuf.substr(loc-5,5)=="fbid=") return true;
     return false;
}

#define SCANNER "scan_acct"
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
BASEEF  ([123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz])
XBASEF  ([^123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz])

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

[^0-9a-z][13]({BASEEF}{27,34})/{XBASEF} {
    accts_scanner &s = *yyaccts_get_extra(yyscanner);
    if(valid_bitcoin_address(yytext+1,yyleng-1)){
       s.pii_recorder->write_buf(SBUF,s.pos+1,yyleng-1);       
    }
    s.pos += yyleng;
}

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
    if(valid_ccn(yytext+1,yyleng-1)){
        s.ccn_recorder->write_buf(SBUF,s.pos+1,yyleng-1);
    }	
    s.pos += yyleng;
}

[^0-9a-z\.][3][0-9][0-9][0-9]{DELIM}[0-9]{6}{DELIM}[0-9]{5}/{END} {
    /* 3### ###### ######### --- 15 digits beginning with 3 and funny space. */
    /* REGEX3 */
    /* Must be american express... */ 
    accts_scanner &s = *yyaccts_get_extra(yyscanner);
    if(valid_ccn(yytext+1,yyleng-1)){
        s.ccn_recorder->write_buf(SBUF,s.pos+1,yyleng-1);
    }	
    s.pos += yyleng;
}

[^0-9a-z\.][3]([0-9]{14})/{END} {
    /* 3### ###### ######### --- 15 digits beginning with 3 and funny space. */
    /* REGEX4 */
    /* Must be american express... */ 
    accts_scanner &s = *yyaccts_get_extra(yyscanner);
    if(valid_ccn(yytext+1,yyleng-1) && !is_fbid(SBUF,s.pos+1) ){
        s.ccn_recorder->write_buf(SBUF,s.pos+1,yyleng-1);
    }	
    s.pos += yyleng;
}

[^0-9a-z\.][3-6]([0-9]{15,18})/{END} {
    /* REGEX5 */
    /* ###############  13-19 numbers as a block beginning with a 3, 4, 5 or 6.
     * followed by something that is not a digit.
     * Yes, CCNs can now be up to 19 digits long. 
     * http://www.creditcards.com/credit-card-news/credit-card-appearance-1268.php
     */
    accts_scanner &s = *yyaccts_get_extra(yyscanner);
    if(valid_ccn(yytext+1,yyleng-1) && !is_fbid(SBUF,s.pos+1)){
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
    if(valid_ccn(yytext+1,16)){  /* validate the first 16 digits */
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
    if(valid_phone(SBUF,s.pos+1,yyleng-1)){
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
        if(valid_phone(SBUF,s.pos+1,yyleng-1)){
            s.telephone_recorder->write_buf(SBUF,s.pos+1,yyleng-1);
        }
    }
    s.pos += yyleng;
}

{PHONETEXT}[0-9/ .+]{7,18} {
    /* REGEX10 */
    /* Generalized number with prefix */
    accts_scanner &s = *yyaccts_get_extra(yyscanner);
    s.telephone_recorder->write_buf(SBUF,s.pos+1,yyleng-1);
    s.pos += yyleng;
}

{PHONETEXT}[0-9 +]+[ ]?[(][0-9]{2,4}[)][ ]?[\-0-9]{4,8} {
    /* REGEX11 */
    /* Generalized number with city code and prefix */
    accts_scanner &s = *yyaccts_get_extra(yyscanner);
    s.telephone_recorder->write_buf(SBUF,s.pos+1,yyleng-1);
    s.pos += yyleng;
}

[ \t\n][+][1-9]([0-9]{10,18})/[^0-9] {
     /* Experimental Regex to find phone numbers beginning with a + */
     /* Phone numbers can be a maximum of 15 digits */
    accts_scanner &s = *yyaccts_get_extra(yyscanner);
    s.telephone_recorder->write_buf(SBUF,s.pos+1,yyleng-1);
    s.pos += yyleng;
}

[ \t\n][0]([1-9][0-9]{9,15})/[^0-9] {
     /* Experimental Regex to find phone numbers beginning with a 0 */
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
    std::wstring stmp;
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
    s.pii_recorder->write_buf(SBUF,s.pos,yyleng);
    s.pos += yyleng;
}

ssn:?[ \t]*[0-9][0-9][0-9]-?[0-9][0-9]-?[0-9][0-9][0-9][0-9]/{END}	{
    /* REGEX13 */
    accts_scanner &s = *yyaccts_get_extra(yyscanner);
    s.pii_recorder->write_buf(SBUF,s.pos,yyleng);
    s.pos += yyleng;
}

[^0-9][0-9][0-9][0-9]-[0-9][0-9]-[0-9][0-9][0-9][0-9]/{END}	{
    /* REGEX13 --- SSNs without the SSN prefix, ssn_mode > 0*/
    accts_scanner &s = *yyaccts_get_extra(yyscanner);
    if(ssn_mode>0){
        s.pii_recorder->write_buf(SBUF,s.pos+1,yyleng-1);
    }
    s.pos += yyleng;
}

[^0-9][0-9][0-9][0-9][0-9][0-9][0-9][0-9][0-9][0-9]/{END} {
    /* REGEX13 --- SSNs without the SSN prefix, with no dashes, ssn_mode > 0
     * basically, these are just 10 digit numbers...
     */
    accts_scanner &s = *yyaccts_get_extra(yyscanner);
    if(ssn_mode>1){
        s.pii_recorder->write_buf(SBUF,s.pos+1,yyleng-1);
    }
    s.pos += yyleng;
}

dob:?[ \t]+{DATEFORMAT}	{
    /* REGEX14 */
    accts_scanner &s = *yyaccts_get_extra(yyscanner);
    s.pii_recorder->write_buf(SBUF,s.pos,yyleng);
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

CT[.](Send|Receive)[.]CMD_([A-Z0-9_]{4,25})[ ]From=([0-9]{2,12}+)(([ ]To=([0-9]{2,12}+))?) {
    /* TeamViewer */
    accts_scanner &s = *yyaccts_get_extra(yyscanner);
    s.pii_recorder->write_buf(SBUF,s.pos,yyleng);
    s.pos += yyleng;
}

sin:?[ \t]*[0-9][0-9][0-9][ -]?[0-9][0-9][0-9][ -]?[0-9][0-9][0-9]/{END} {
    /* Canadian SIN with prefix, optional dashes or spaces */
    accts_scanner &s = *yyaccts_get_extra(yyscanner);
    s.sin_recorder->write_buf(SBUF,s.pos,yyleng);
    s.pos += yyleng;
}

[^0-9][0-9][0-9][0-9]-[0-9][0-9][0-9]-[0-9][0-9][0-9]/{END} {
    /* Canadian SIN without prefix, dashes required */
    accts_scanner &s = *yyaccts_get_extra(yyscanner);
    s.sin_recorder->write_buf(SBUF,s.pos+1,yyleng-1);
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
    if(sp.phase==scanner_params::PHASE_STARTUP){
        assert(sp.info->si_version==scanner_info::CURRENT_SI_VERSION);
	sp.info->name  = "accts";
	sp.info->author		= "Simson L. Garfinkel, modified by Tim Walsh";
	sp.info->description	= "scans for CCNs, track 2, PII (including SSN and Canadian SIN), and phone #s";
	sp.info->scanner_version= "1.0";
        sp.info->feature_names.insert("ccn");
        sp.info->feature_names.insert("pii");  // personally identifiable information
        sp.info->feature_names.insert("sin");
        sp.info->feature_names.insert("ccn_track2");
        sp.info->feature_names.insert("telephone");
	sp.info->histogram_defs.insert(histogram_def("ccn","","histogram"));
	sp.info->histogram_defs.insert(histogram_def("ccn_track2","","histogram"));
	sp.info->histogram_defs.insert(histogram_def("telephone","","histogram",HistogramMaker::FLAG_NUMERIC));
        sp.info->histogram_defs.insert(histogram_def("pii","CT.*CMD_.*((From|To)=[0-9]+)","teamviewer",HistogramMaker::FLAG_NUMERIC));
        sp.info->get_config("ssn_mode",&ssn_mode,"0=Normal; 1=No `SSN' required; 2=No dashes required");
        sp.info->get_config("min_phone_digits",&min_phone_digits,"Min. digits required in a phone");
        scan_ccns2_debug = sp.info->config->debug;           // get debug value
        build_unbase58();
	return;
    }
    if(sp.phase==scanner_params::PHASE_SCAN){
        accts_scanner lexer(sp);
	yyscan_t scanner;
        yyaccts_lex_init(&scanner);
	yyaccts_set_extra(&lexer,scanner);
        try {
            yyaccts_lex(scanner);
        }
        catch (sbuf_scanner::sbuf_scanner_exception *e ) {
            std::cerr << "Scanner " << SCANNER << "Exception " << e->what() << " processing " << sp.sbuf.pos0 << "\n";
            delete e;
        }
                
        yyaccts_lex_destroy(scanner);
    }
    if(sp.phase==scanner_params::PHASE_NONE){                 // avoids defined but not used
	(void)yyunput;			
        (void)yy_fatal_error;
    }
}
