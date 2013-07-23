#include <iostream>
#include <string.h>

#include "isomf.h"
#include "isomf_factory.h"
#include "type_io.h"

using namespace std;
using namespace isomf;

/*
void usage(char* me) {
  cout << me << ": [input_file]" << endl;
}
*/

int main(int argc, char** argv) {
  picture_parameter_set* pps = parameter_set_factory::get_avc_pps();
  cout << pps->str() << endl;
  cout << "PPS size: " << pps->size() << endl;

  size_t sz = 1024;
  char buf[sz];
  memset(buf, 0, sz);

  size_t nb = pps->write(buf, sz);
  cout << "Written " << nb << " bytes into buffer.";

  type_io::print_hex(buf, nb);

  return 0;
}
