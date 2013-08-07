%{
/* scan_flexdemo.flex
 * 
 *
 * A very simple flex scanner
 */

#include "config.h"
#include "be13_api/bulk_extractor_i.h"
#include "sbuf_flex_scanner.h"

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define FEATURE_NAME "demo"

class flex_scanner : public sbuf_scanner {
private:
      flex_scanner(const flex_scanner &that):sbuf_scanner(that.sbuf),demo_recorder(){}
      flex_scanner &operator=(const flex_scanner &that){return *this;}
public:
      flex_scanner(const scanner_params &sp):
        sbuf_scanner(&sp.sbuf),demo_recorder(){
          demo_recorder  = sp.fs.get_name(FEATURE_NAME);
      }
      virtual ~flex_scanner(){}
      class feature_recorder *demo_recorder;
};
#define YY_EXTRA_TYPE flex_scanner *             /* holds our class pointer */

inline class flex_scanner *get_extra(yyscan_t yyscanner) {
  /* placing decl here avoids redundent declaration warning */
   YY_EXTRA_TYPE yyflexdemo_get_extra (yyscan_t yyscanner );   
  return yyflexdemo_get_extra(yyscanner);
}

%}

%option reentrant
%option noyywrap
%option 8bit
%option batch
%option case-insensitive
%option pointer
%option noyymore
%option prefix="yyflexdemo_"

%%

"[ ]"+demo[0-9]*/"[ ]"	{
    /** Look for the word demo preceeded and followed by a space. 
     ** Note that we are looking for one letter more than we care capturing, so 
     ** we need to add a character to s.pos and remove one from yyleng
     **
     ** NEVER modify yyleng, or else pos will not reflect the current position in the buffer.
     */
    flex_scanner &s = * yyflexdemo_get_extra(yyscanner);
    s.demo_recorder->write_buf(SBUF,s.pos+1,yyleng-1);
    s.pos += yyleng; 

}

.|\n { 
     /**
      * The no-match rule. VERY IMPORTANT!
      */
     flex_scanner &s = *yyflexdemo_get_extra(yyscanner);	      
     s.pos++; 
}
%%

extern "C"
void scan_flexdemo(const class scanner_params &sp,const recursion_control_block &rcb)
{
    assert(sp.sp_version==scanner_params::CURRENT_SP_VERSION);      
    if(sp.phase==scanner_params::PHASE_STARTUP){
        assert(sp.info->si_version==scanner_info::CURRENT_SI_VERSION);
	sp.info->name		= "flexdemo";
        sp.info->author         = "Simson L. Garfinkel";
        sp.info->description    = "Scans for the word 'demo' optionally followed by a number with gnuflex";
        sp.info->scanner_version= "1.0";

	/* define the feature files this scanner created */
        sp.info->feature_names.insert(FEATURE_NAME);

	/* define the histograms to make */
	sp.info->histogram_defs.insert(histogram_def(FEATURE_NAME,"","histogram"));
	return;
    }
    if(sp.phase==scanner_params::PHASE_SHUTDOWN){
        return; 
    }
    if(sp.phase==scanner_params::PHASE_SCAN){
	/* Set up the buffer. Scan it. Exit */
	flex_scanner lexer(sp);
	yyscan_t scanner;
        yyflexdemo_lex_init(&scanner);
	yyflexdemo_set_extra(&lexer,scanner);
	yyflexdemo_lex(scanner);
        yyflexdemo_lex_destroy(scanner);
	(void)yyunput;			// avoids defined but not used
    }
}

