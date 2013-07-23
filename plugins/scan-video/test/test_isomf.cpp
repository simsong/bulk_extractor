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
  /*
  if (argc < 2) {
    usage(argv[0]);
    return 0;
  }
  */
  sample_description_box* box = NULL;

  box = box_factory::get_avc_stsd(320, 240);
  box_factory::print_stsd(box);

  box = box_factory::get_mp4a_stsd();
  box_factory::print_stsd(box);

  box = box_factory::get_mp4v_stsd(176, 144);
  box_factory::print_stsd(box);

  box = box_factory::get_h263_stsd(176, 144);
  box_factory::print_stsd(box);

  return 0;
}
