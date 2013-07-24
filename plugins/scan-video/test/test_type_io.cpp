#include <iostream>
#include <string.h>

#include "type_io.h"
using namespace std;

int main(int argc, char** argv) {

  char buf[8];
  int n = (1 << 24) + (2 << 16) + (3 << 8) + 4;

  type_io::write_uint32(n, buf);
  type_io::print_hex(buf, 4);

  for (int i=1; i<=8; i++) {
    memset(buf, 0, 8);
    type_io::write_unsigned(n, buf, i);
    type_io::print_hex(buf, 8);
  }

  return 0;
}
