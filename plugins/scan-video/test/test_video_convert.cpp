
#include <iostream>
#include <opencv2/opencv.hpp>
#include "video_processor.h"

using namespace cv;
using namespace std;

void usage(char* me) {
  cout << me << ": [input_file] [output_file]" << endl;
}

int main(int argc, char** argv) {
  if (argc < 3) {
    usage(argv[0]);
    return 0;
  }

  video_processor p;
  video_info info;
  p.convert(argv[1], argv[2], info, video_processor::CODEC_MP4);

  return 0;
}
