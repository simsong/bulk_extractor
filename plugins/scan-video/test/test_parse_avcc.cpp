#include <iostream>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "isomf.h"

using namespace std;
using namespace isomf;

void usage(char* me)
{
  cerr << "Usage: " << me << " raw_avcc_box_dump" << endl;
}

char* read_file(const char* filename, size_t* filesize)
{
  if (! filename) { return NULL; }

  FILE* fp = fopen(filename, "r");
  if (! fp) {
    cerr << "Failed to open " << filename << endl;
    return NULL;
  }

  if (fseek(fp, 0, SEEK_END)) {
    cerr << "Failed to seek " << filename << endl;
    fclose(fp);
    return NULL;
  }
  long sz = ftell(fp);
  *filesize = (size_t)sz;
  rewind(fp);

  char* buf = new char[sz];

  size_t nb = fread(buf, 1, sz, fp);
  fclose(fp);

  if (nb < (size_t) sz) {
    cerr << "Failed to read " << filename << endl;
    return NULL;
  }

  cerr << "Read " << nb << " bytes from " << filename << endl;

  return buf;
}

int main(int argc, char** argv)
{
  if (argc < 2) {
    usage(argv[0]);
    return 1;
  }
  char* filename = argv[1];
  size_t filesize = 0;

  char* buf = read_file(filename, &filesize);
  if (! buf) { return 1; }

  avc_configuration_box avcc;
  size_t nb = avcc.parse(buf, filesize);
  cerr << "Parsed " << nb << " bytes from " << filename << endl;

  list<sequence_parameter_set*>& sps_list = avcc.get_sps_list();
  list<picture_parameter_set*>& pps_list = avcc.get_pps_list();

  list<sequence_parameter_set*>::iterator itr = sps_list.begin();
  while (itr != sps_list.end()) {
    cout << (*itr)->str() << endl;
    itr++;
  }

  list<picture_parameter_set*>::iterator pps_itr = pps_list.begin();
  while (pps_itr != pps_list.end()) {
    cout << (*pps_itr)->str() << endl;
    pps_itr++;
  }

  return 0;
}

