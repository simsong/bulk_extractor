
#include <iostream>
#include <opencv2/opencv.hpp>
#include "video_processor.h"

using namespace cv;
using namespace std;

void usage(char* me) {
  cout << me << ": [video_file] ..." << endl;
}

int main(int argc, char** argv) {
  if (argc < 2) {
    usage(argv[0]);
    return 0;
  }

  video_info info;  
  video_processor p;
  for (int i=1; i<argc; i++) {
    if (! p.get_info(argv[i], info)) {
      continue;
    }

    cout << argv[i] << endl;
    cout << "\tcodec: " << info.codec_str() << endl;
    cout << "\tsize: " << info.width << "x" << info.height << endl;
    cout << "\tfps: " << info.fps << endl;
  }

  return 0;
}
