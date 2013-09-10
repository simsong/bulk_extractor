#include <string>

namespace base16 {
  //
  // patterns
  //

  // FIXME: trailing context
  // FIXME: leading context
  /* 
   * hex with junk before it.
   * {0,4} means we have 0-4 space characters
   * {6,}  means minimum of 6 hex bytes
   */
  const std::string HEX("[^0-9A-F]([0-9A-F]{2}[ \\t\\n\\r]{0,4}){6,}[^0-9A-F]");
}
