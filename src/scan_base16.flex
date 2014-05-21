%{

/*
 * http://flex.sourceforge.net/manual/Cxx.html
 */

#define SCANNER "scan_base16"

static const int BASE16_IGNORE  = -2;
static const int BASE16_INVALID = -1;
static int base16array[256];

unsigned int opt_min_hex_buf = 64;           /* Don't re-analyze hex bufs smaller than this */

#include "config.h"
#include "be13_api/bulk_extractor_i.h"
#include "sbuf_flex_scanner.h"

class base16_scanner : public sbuf_scanner {
public:
    base16_scanner(const scanner_params &sp_,const recursion_control_block &rcb_):
        sbuf_scanner(&sp_.sbuf),sp(sp_),rcb(rcb_),hex_recorder(sp.fs.get_name("hex")){
    }
    
    const class scanner_params &sp;    
    const class recursion_control_block &rcb;
    class feature_recorder *hex_recorder;
    void  decode(const sbuf_t &osbuf);
};
#define YY_EXTRA_TYPE base16_scanner *   /* holds our class pointer */
YY_EXTRA_TYPE yybase16_get_extra (yyscan_t yyscanner );    /* redundent declaration */
inline class base16_scanner *get_extra(yyscan_t yyscanner) {return yybase16_get_extra(yyscanner);}


void base16_scanner::decode(const sbuf_t &sbuf)
{
    managed_malloc<uint8_t>b(sbuf.pagesize/2+1);
    if(b.buf==0) return;

    size_t p=0;
    /* First get the characters */
    for(size_t i=0;i+1<sbuf.pagesize;){
        /* stats on the new characters */

        /* decode the two characters */
        int msb = base16array[sbuf[i]];
        if(msb==BASE16_IGNORE || msb==BASE16_INVALID){
            i++;          /* This character not valid */
            continue;
        }
        assert(msb>=0 && msb<16);
        int lsb = base16array[sbuf[i+1]];
        if(lsb==BASE16_IGNORE || lsb==BASE16_INVALID){
            return;       /* If first char is valid hex and second isn't, this isn't hex */
        }
        assert(lsb>=0 && lsb<16);
        b.buf[p++] = (msb<<4) | lsb;
        i+=2;
    }

    /* Alert on byte sequences of 48, 128 or 256 bits*/
    if(p==48/8 || p==128/8 || p==256/8){
        hex_recorder->write_buf(sbuf,0,sbuf.bufsize);  /* it validates; write original with context */
        return;                                  /* Small keys don't get recursively analyzed */
    }
    if(p>opt_min_hex_buf){
        sbuf_t nsbuf(sbuf.pos0 + rcb.partName,b.buf,p,p,false);
        (*rcb.callback)(scanner_params(sp,nsbuf)); // recurse
    }
}


%}

%option noyywrap
%option 8bit
%option batch
%option case-insensitive
%option pointer
%option noyymore
%option prefix="yybase16_"


UNICODE         ([[:print:][:space:]]+)

%%

[0-9A-F]{2}(([ \x0A]|\x0D\x0A){0,2}[0-9A-F]{2}){5,}     {
    /*** WARNING:
     *** DO NOT USE "%option fast" ABOVE.
     *** IT GENERATES ADDRESS SANITIZER ERRORS IN THE LEXER.
     *** SIMSON GARFINKEL, MAY 15, 2014
     ***/

    /* hex with junk before it.
     * {0,4} means we have 0-4 space characters
     * {6,65536}  means 6-65536 characters
     */
    base16_scanner &s = *yybase16_get_extra(yyscanner);
    s.decode(sbuf_t(s.sp.sbuf, s.pos, yyleng));
    s.pos += yyleng;
}
 
.|\n { 
     /**
      * The no-match rule.
      * If we are beyond the end of the margin, call it quits.
      */
    sbuf_scanner &s = *yybase16_get_extra(yyscanner);
    s.pos++; 
}
%%

extern "C"
void scan_base16(const class scanner_params &sp,const recursion_control_block &rcb)
{
    static const u_char *ignore_string = (const u_char *)"\r\n \t";
    assert(sp.sp_version==scanner_params::CURRENT_SP_VERSION);      
    if(sp.phase==scanner_params::PHASE_STARTUP){
        assert(sp.info->si_version==scanner_info::CURRENT_SI_VERSION);

        sp.info->name           = "base16";
        sp.info->author         = "Simson L. Garfinkel";
        sp.info->description    = "Base16 (hex) scanner";
        sp.info->scanner_version= "1.0";
        sp.info->feature_names.insert("hex"); // notable hex values
        sp.info->flags          = scanner_info::SCANNER_DISABLED | scanner_info::SCANNER_RECURSE;

        /* Create the base16 array */
        for(int i=0;i<256;i++){
            base16array[i] = BASE16_INVALID;
        }
        for(const u_char *ch = ignore_string;*ch;ch++){
            base16array[(int)*ch] = BASE16_IGNORE;
        }
        for(int ch='A';ch<='F';ch++){ base16array[ch] = ch-'A'+10; }
        for(int ch='a';ch<='f';ch++){ base16array[ch] = ch-'a'+10; }
        for(int ch='0';ch<='9';ch++){ base16array[ch] = ch-'0'; }
        return; /* No feature files created */
    }
    if(sp.phase==scanner_params::PHASE_SCAN){
        if(sp.sbuf.pagesize<24) return; /* minimum size to scan */
        yyscan_t scanner;
        yybase16_lex_init(&scanner);

        {
                base16_scanner lexer(sp,rcb);
                yybase16_set_extra(&lexer,scanner);
                yybase16_lex(scanner);
        }


        yybase16_lex_destroy(scanner);
    }
}

void scan_base16_ignore_me()
{
        (void)yyunput;                  // avoids defined but not used
}
