#include "config.h"

// if liblightgrep isn't present, compiles to nothing
#ifdef HAVE_LIBLIGHTGREP

#include <string>

#include "be13_api/bulk_extractor_i.h"
#include "pattern_scanner.h"

namespace gps {
  const char* const DefaultEncodingsCStrings[] = {"UTF-8", "UTF-16LE"};

  const vector<string> DefaultEncodings(
    &DefaultEncodingsCStrings[0], &DefaultEncodingsCStrings[2]
  );

  const LG_KeyOptions DefaultOptions = { 0, 0 }; // patterns, case-sensitive

  //
  // helper functions
  //

  /**
   * Return NNN in <tag attrib="NNN">
   */
  string get_quoted_attrib(string text, string attrib) {
    size_t pos = text.find(attrib);
    if (pos == string::npos) return "";  /* no attrib */
    ssize_t quote1 = text.find('"', pos);
    if (quote1 < 0) return "";           /* no opening quote */
    ssize_t quote2 = text.find('"', quote1+1);
    if (quote2 < 0) return "";           /* no closing quote */
    return text.substr(quote1+1, quote2-(quote1+1));
  }

  /**
   * Return NNN in <tag>NNN</tag>
   */
  string get_cdata(string text) {
    ssize_t gt = text.find('>');
    if (gt < 0) return "";           /* no > */
    ssize_t lt = text.find('<', gt+1);
    if (lt < 0) return "";           /* no < */
    return text.substr(gt+1, lt-(gt+1));
  }

  /**
   * dump the current and go to the next
   */
/*
  void clear() {
    if (time.size() || lat.size() || lon.size() || ele.size() || speed.size() || course.size()) {
      string what = time+","+lat+","+lon+","+ele+","+speed+","+course;
      gps_recorder->write(sbuf->pos0+pos,what,"");
    }
    time = "";
    lat = "";
    lon = "";
    ele = "";
    speed = "";
    course = "";
  }
*/

  //
  // subpatterns
  //

  const std::string LATLON("(-?[0-9]{1,3}\\.[0-9]{6,8})");
  const std::string ELEV("(-?[0-9]{1,6}\\.[0-9]{0,3})");

  //
  // the scanner
  //

  class Scanner: public PatternScanner {
  public:
    Scanner(): PatternScanner("gps_lg"), Recorder(0) {}
    virtual ~Scanner() {}

    virtual Scanner* clone() const { return new Scanner(*this); }

    virtual void startup(const scanner_params& sp);
    virtual void init(const scanner_params& sp);
    virtual void initScan(const scanner_params&);

    feature_recorder* Recorder;

    void trkptHandler(const LG_SearchHit& hit, const scanner_params& sp, const recursion_control_block& rcb);

    void eleHandler(const LG_SearchHit& hit, const scanner_params& sp, const recursion_control_block& rcb);

    void timeHandler(const LG_SearchHit& hit, const scanner_params& sp, const recursion_control_block& rcb);

    void speedHandler(const LG_SearchHit& hit, const scanner_params& sp, const recursion_control_block& rcb);
 
    void courseHandler(const LG_SearchHit& hit, const scanner_params& sp, const recursion_control_block& rcb);

  private:
    Scanner(const Scanner& s): PatternScanner(s), Recorder(s.Recorder) {}
    Scanner& operator=(const Scanner&);
  };

  void Scanner::startpup(const scanner_params& sp) {
    assert(sp.sp_version == scanner_params::CURRENT_SP_VERSION);      
    assert(sp.info->si_version == scanner_info::CURRENT_SI_VERSION);

    sp.info->name            = "gps_lg";
    sp.info->author          = "Simson L. Garfinkel";
    sp.info->description     = "Garmin Trackpt XML info";
    sp.info->scanner_version = "1.0";
    sp.info->feature_names.insert("gps");
  }

  void Scanner::init(const scanner_params& sp) {
    //
    // patterns
    //

    const std::string TRKPT("<trkpt lat=\"" + LATLON + "\" lon=\"" + LATLON + '"');

    new Handler(
      *this,
      TRKPT,
      DefaultEncodings,
      DefaultOptions,
      &Scanner::trkptHandler
    );

    const std::string ELE("<ele>" + ELEV + "</ele>");

    new Handler(
      *this,
      TRKPT,
      DefaultEncodings,
      DefaultOptions,
      &Scanner::eleHandler
    );

    const std::string TIME("<time>[0-9]{4}(-[0-9]{2}){2}[ T]([0-9]{2}:){2}[0-9]{2}(Z|([-+][0-9.]))</time>");

    new Handler(
      *this,
      TRKPT,
      DefaultEncodings,
      DefaultOptions,
      &Scanner::timeHandler
    );

    const std::string GPXTPX_SPEED("<gpxtpx:speed>" + ELEV + "</gpxtpx:speed>");

    new Handler(
      *this,
      TRKPT,
      DefaultEncodings,
      DefaultOptions,
      &Scanner::speedHandler
    );

    const std::string GPXTPX_COURSE("<gpxtpx:course>" + ELEV + "</gpxtpx:course>");
    
    new Handler(
      *this,
      TRKPT,
      DefaultEncodings,
      DefaultOptions,
      &Scanner::courseHandler
    );
  }

  void Scanner::initScan(const scanner_params& sp) {
    Recorder = sp.fs.get_name("gps"); 
  }

  void Scanner::trkptHandler(const LG_SearchHit& hit, const scanner_params& sp, const recursion_control_block& rcb) {
  }

  void Scanner::eleHandler(const LG_SearchHit& hit, const scanner_params& sp, const recursion_control_block& rcb) {
  }

  void Scanner::timeHandler(const LG_SearchHit& hit, const scanner_params& sp, const recursion_control_block& rcb) {
  }

  void Scanner::speedHandler(const LG_SearchHit& hit, const scanner_params& sp, const recursion_control_block& rcb) {
  }
 
  void Scanner::courseHandler(const LG_SearchHit& hit, const scanner_params& sp, const recursion_control_block& rcb) {
  }

  Scanner TheScanner;
}

extern "C"
void scan_gps_lg(const class scanner_params &sp, const recursion_control_block &rcb) {
  scan_lg(gps::TheScanner, sp, rcb);
}

#endif // HAVE_LIBLIGHTGREP
