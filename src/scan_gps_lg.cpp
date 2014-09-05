#include "config.h"

// if liblightgrep isn't present, compiles to nothing
#ifdef HAVE_LIBLIGHTGREP

#include <string>

#include "be13_api/bulk_extractor_i.h"
#include "pattern_scanner.h"

namespace gps {
  const char* const DefaultEncodingsCStrings[] = {"UTF-8", "UTF-16LE"};

  const vector<string> DefaultEncodings(
    DefaultEncodingsCStrings,
    DefaultEncodingsCStrings +
      sizeof(DefaultEncodingsCStrings)/sizeof(DefaultEncodingsCStrings[0])
  );

  const LG_KeyOptions DefaultOptions = { 0, 0 }; // patterns, case-sensitive

  //
  // helper functions
  //

  /**
   * Return NNN in <tag attrib="NNN">
   */
  string get_quoted_attrib(string text, string attrib) {
    const size_t pos = text.find(attrib);
    if (pos == string::npos) return "";  /* no attrib */
    const ssize_t quote1 = text.find('"', pos);
    if (quote1 < 0) return "";           /* no opening quote */
    const ssize_t quote2 = text.find('"', quote1+1);
    if (quote2 < 0) return "";           /* no closing quote */
    return text.substr(quote1+1, quote2-(quote1+1));
  }

  /**
   * Return NNN in <tag>NNN</tag>
   */
  string get_cdata(string text) {
    const ssize_t gt = text.find('>');
    if (gt < 0) return "";           /* no > */
    const ssize_t lt = text.find('<', gt+1);
    if (lt < 0) return "";           /* no < */
    return text.substr(gt+1, lt-(gt+1));
  }

  //
  // subpatterns
  //

  const string LATLON("(-?[0-9]{1,3}\\.[0-9]{6,8})");
  const string ELEV("(-?[0-9]{1,6}\\.[0-9]{0,3})");

  //
  // the scanner
  //

  class Scanner: public PatternScanner {
  public:
    Scanner(): PatternScanner("gps_lg"), Recorder(0), Lat(), Lon(), Ele(), Time(), Speed(), Course() {}
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
    Scanner(const Scanner& s): PatternScanner(s), Recorder(s.Recorder), Lat(s.Lat), Lon(s.Lon), Ele(s.Ele), Time(s.Time), Speed(s.Speed), Course(s.Course) {}
    Scanner& operator=(const Scanner&);

    void clear(const scanner_params& sp, size_t pos);

    string Lat, Lon, Ele, Time, Speed, Course;
  };

  void Scanner::startup(const scanner_params& sp) {
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

    const string TRKPT("<trkpt lat=\"" + LATLON + "\" lon=\"" + LATLON + '"');

    new Handler(
      *this,
      TRKPT,
      DefaultEncodings,
      DefaultOptions,
      &Scanner::trkptHandler
    );

    const string ELE("<ele>" + ELEV + "</ele>");

    new Handler(
      *this,
      ELE,
      DefaultEncodings,
      DefaultOptions,
      &Scanner::eleHandler
    );

    const string TIME("<time>[0-9]{4}(-[0-9]{2}){2}[ T]([0-9]{2}:){2}[0-9]{2}(Z|([-+][0-9.]))</time>");

    new Handler(
      *this,
      TIME,
      DefaultEncodings,
      DefaultOptions,
      &Scanner::timeHandler
    );

    const string GPXTPX_SPEED("<gpxtpx:speed>" + ELEV + "</gpxtpx:speed>");

    new Handler(
      *this,
      GPXTPX_SPEED,
      DefaultEncodings,
      DefaultOptions,
      &Scanner::speedHandler
    );

    const string GPXTPX_COURSE("<gpxtpx:course>" + ELEV + "</gpxtpx:course>");
    
    new Handler(
      *this,
      GPXTPX_COURSE,
      DefaultEncodings,
      DefaultOptions,
      &Scanner::courseHandler
    );
  }

  void Scanner::initScan(const scanner_params& sp) {
    Recorder = sp.fs.get_name("gps"); 
  }

  void Scanner::clear(const scanner_params& sp, size_t pos) {
    // dump the current and go to the next
    if (!Time.empty() || !Lat.empty() || !Lon.empty() ||
        !Ele.empty() || !Speed.empty() || !Course.empty()) {
      const string what = Time + "," + Lat + "," + Lon + "," +
                          Ele + "," + Speed + "," + Course;
      // NB: the pos is the *end* of the "hit"
      Recorder->write(sp.sbuf.pos0 + pos, what, "");

      Time.clear();
      Lat.clear();
      Lon.clear();
      Ele.clear();
      Speed.clear();
      Course.clear();
    }
  }

  void Scanner::trkptHandler(const LG_SearchHit& hit, const scanner_params& sp, const recursion_control_block& rcb) {
    clear(sp, hit.Start);
    Lat = get_quoted_attrib(reinterpret_cast<const char*>(sp.sbuf.buf), "lat");
    Lon = get_quoted_attrib(reinterpret_cast<const char*>(sp.sbuf.buf), "lon");
  }

  void Scanner::eleHandler(const LG_SearchHit& hit, const scanner_params& sp, const recursion_control_block& rcb) {
    Ele = get_cdata(reinterpret_cast<const char*>(sp.sbuf.buf));
  }

  void Scanner::timeHandler(const LG_SearchHit& hit, const scanner_params& sp, const recursion_control_block& rcb) {
    Time = get_cdata(reinterpret_cast<const char*>(sp.sbuf.buf));
  }

  void Scanner::speedHandler(const LG_SearchHit& hit, const scanner_params& sp, const recursion_control_block& rcb) {
    Speed = get_cdata(reinterpret_cast<const char*>(sp.sbuf.buf));
  }
 
  void Scanner::courseHandler(const LG_SearchHit& hit, const scanner_params& sp, const recursion_control_block& rcb) {
    Course = get_cdata(reinterpret_cast<const char*>(sp.sbuf.buf));
  }

  Scanner TheScanner;
}

extern "C"
void scan_gps_lg(const class scanner_params &sp, const recursion_control_block &rcb) {
  scan_lg(gps::TheScanner, sp, rcb);
}

#endif // HAVE_LIBLIGHTGREP
