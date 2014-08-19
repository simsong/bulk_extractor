%{
/* scan_gps.flex:
 * A flex-based scanner to pick up GPS coordinates.
 * Currently supports Garmin Trackpt GPX format.
 * Note that we could do this with an XML parser, but that wouldn't pick up fragments
 * within memory
 *
 */

/*
 * Here are samples that we crib from:
 *
 * A garmin Trackpoint from a Nuvi 240
 *

 <trkpt lat="38.866110" lon="-77.136286"><ele>90.98</ele><time>2010-10-16T20:46:52Z</time><extensions><gpxtpx:TrackPointExtension><gpxtpx:speed>21.96</gpxtpx:speed><gpxtpx:course>87.53</gpxtpx:course></gpxtpx:TrackPointExtension></extensions></trkpt>

 */

#include "config.h"
#include "be13_api/bulk_extractor_i.h"

#include <stdlib.h>
#include <string.h>

#include "sbuf_flex_scanner.h"
class gps_scanner : public sbuf_scanner {
      /* Standards for all flex lexers */
      public:
      gps_scanner(const scanner_params &sp): sbuf_scanner(&sp.sbuf),
        gps_recorder(),lat(),lon(),ele(),time(),speed(),course(){
           gps_recorder = sp.fs.get_name("gps");
        }

      static std::string get_quoted_attrib(std::string text,std::string attrib);
      static std::string get_cdata(std::string text);
      void clear();

      class feature_recorder *gps_recorder;
      std::string lat;
      std::string lon;
      std::string ele;
      std::string time;
      std::string speed;
      std::string course;
};

#define YY_EXTRA_TYPE gps_scanner *         /* holds our class pointer */
YY_EXTRA_TYPE yygps_get_extra (yyscan_t yyscanner );    /* redundent declaration */
inline class gps_scanner *get_extra(yyscan_t yyscanner) {return yygps_get_extra(yyscanner);}

/**
 * Return NNN in <tag attrib="NNN">
 */

std::string gps_scanner::get_quoted_attrib(std::string text,std::string attrib)
{
        size_t pos = text.find(attrib);
        if(pos==std::string::npos) return "";  /* no attrib */
        ssize_t quote1 = text.find('\"',pos);
        if(quote1<0) return "";           /* no opening quote */
        ssize_t quote2 = text.find('\"',quote1+1);
        if(quote2<0) return "";           /* no closing quote */
        return text.substr(quote1+1,quote2-(quote1+1));
}

/**
 * Return NNN in <tag>NNN</tag>
 */

std::string gps_scanner::get_cdata(std::string text)
{
        ssize_t gt = text.find('>');
        if(gt<0) return "";           /* no > */
        ssize_t lt = text.find('<',gt+1);
        if(lt<0) return "";           /* no < */
        return text.substr(gt+1,lt-(gt+1));
}

/**
 * dump the current and go to the next
 */
void gps_scanner::clear()
{
        if(time.size() || lat.size() || lon.size() || ele.size() || speed.size() || course.size()){
                std::string what = time+","+lat+","+lon+","+ele+","+speed+","+course;
                gps_recorder->write(sbuf->pos0+pos,what,"");
        }
        time = "";
        lat = "";
        lon = "";
        ele = "";
        speed = "";
        course = "";
}

%}

%option noyywrap
%option 8bit
%option batch
%option case-sensitive
%option pointer
%option noyymore
%option prefix="yygps_"

LATLON  (-?[0-9]{1,3}[.][0-9]{6,8})
ELEV    (-?[0-9]{1,6}[.][0-9]{0,3})

%%

[<]trkpt\ lat=\"{LATLON}\"\ lon=\"{LATLON}\"  {
        gps_scanner &s = *yygps_get_extra(yyscanner);
        s.clear();
        s.lat = gps_scanner::get_quoted_attrib(yytext,"lat");
        s.lon = gps_scanner::get_quoted_attrib(yytext,"lon");
        s.pos += yyleng;
}


[<]/trkpt[>] {
        gps_scanner &s = *yygps_get_extra(yyscanner);
        s.clear();
        s.pos += yyleng;
}

[<]ele[>]{ELEV}[<][/]ele[>] {
        gps_scanner &s = *yygps_get_extra(yyscanner);
        s.ele = gps_scanner::get_cdata(yytext);
        s.pos += yyleng; 
}

[<]time[>][0-9][0-9][0-9][0-9]-[0-9][0-9]-[0-9][0-9][ T][0-9][0-9]:[0-9][0-9]:[0-9][0-9](Z|([-+][0-9.]))[<][/]time[>] {
        gps_scanner &s = *yygps_get_extra(yyscanner);
        s.time = gps_scanner::get_cdata(yytext);
        s.pos += yyleng; 
}

[<]gpxtpx:speed[>]{ELEV}[<][/]gpxtpx:speed[>] {
        gps_scanner &s = *yygps_get_extra(yyscanner);
        s.speed = gps_scanner::get_cdata(yytext);
        s.pos += yyleng;
}

[<]gpxtpx:course[>]{ELEV}[<][/]gpxtpx:course[>] {
        gps_scanner &s = *yygps_get_extra(yyscanner);
        s.course = gps_scanner::get_cdata(yytext);
        s.pos += yyleng;
}

.|\n { 
    /**
     * The no-match rule.
     * If we are beyond the end of the margin, call it quits.
     */
    /* putchar(yytext[0]); */ /* Uncomment for debugging */

    /* If we have an invalid character, then we are out of this XML block. Clear */
    gps_scanner &s = *yygps_get_extra(yyscanner);
    if(yytext[0] & 0x80){
        s.clear();
    }
    s.pos++; 
}
%%

extern "C"
void scan_gps(const class scanner_params &sp,const recursion_control_block &rcb)
{
    assert(sp.sp_version==scanner_params::CURRENT_SP_VERSION);      
    if(sp.phase==scanner_params::PHASE_STARTUP){
        assert(sp.info->si_version==scanner_info::CURRENT_SI_VERSION);
        sp.info->name           = "gps";
        sp.info->author         = "Simson L. Garfinkel";
        sp.info->description    = "Garmin Trackpt XML info";
        sp.info->scanner_version= "1.0";
        sp.info->feature_names.insert("gps");
        return;
    }
    if(sp.phase==scanner_params::PHASE_SCAN){
        /* Prescan */
        if(sp.sbuf.find("trkpt",0)==-1 || sp.sbuf.find("lat=",0)==-1 || sp.sbuf.find("lon=",0)==-1) return;

        /* Scan */
        gps_scanner lexer(sp);
        yyscan_t scanner;
        yygps_lex_init(&scanner);
        yygps_set_extra(&lexer,scanner);
        yygps_lex(scanner);
        yygps_lex_destroy(scanner);
        (void)yyunput;                  // avoids defined but not used
    }
}

