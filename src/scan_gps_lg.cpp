#include <string>

namespace gps {
  //
  // subpatterns
  //

  const std::string LATLON("(-?[0-9]{1,3}\\.[0-9]{6,8})");
  const std::string ELEV("(-?[0-9]{1,6}\\.[0-9]{0,3})");

  //
  // patterns
  //

  const std::string TRKPT("<trkpt lat=\"" + LATLON + "\" lon=\"" + LATLON + '"');
  
  const std::string ELE("<ele>" + ELEV + "</ele>");

  const std::string TIME("<time>[0-9]{4}(-[0-9]{2}){2}[ T]([0-9]{2}:){2}[0-9]{2}(Z|([-+][0-9.]))</time>");

  const std::string GPXTPX_SPEED("<gpxtpx:speed>" + ELEV + "</gpxtpx:speed>");

  const std::string GPXTPX_COURSE("<gpxtpx:course>" + ELEV + "</gpxtpx:course>");
}
