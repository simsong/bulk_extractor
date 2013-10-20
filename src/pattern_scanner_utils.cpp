#include "pattern_scanner_utils.h"

string low_utf16le_to_ascii(const uint8_t* buf, size_t len) {
  // When UTF-16LE is restricted to code points <= U+7F,
  // removing every odd byte converts it to ASCII encoding.
  const size_t olen = len/2;
  string out(olen,'\0');

  for (size_t i = 0; i < olen; ++i) {
    out[i] = buf[2*i];
  }

  return out;
}
