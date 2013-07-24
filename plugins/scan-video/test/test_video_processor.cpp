#include <iostream>
#include <opencv2/opencv.hpp>
#include "video_processor.h"

using namespace cv;
using namespace std;


int main(int argc, char** argv) {
  VideoCapture cap(0); // open the default camera
  if(!cap.isOpened())  // check if we succeeded
    return -1;

  Mat frame;
  cap >> frame;

  Size size = frame.size();

  //int codec = CV_FOURCC('M','J','P','G');
  //int codec = CV_FOURCC('I', 'Y', 'U', 'V');
  int codec = CV_FOURCC('m','p','4','v');
  //int codec = CV_FOURCC('X','V','I','D');
  //int codec = CV_FOURCC('X','2','6','4');
  double fps = 10;

  VideoWriter writer;
  writer.open("recording.avi", codec, fps, size, true);
  if (! writer.isOpened()) {
    cerr << "Failed to open video stream \n";
    return -1;
  }

  Mat edges;
  namedWindow("edges",1);
  for(;;)
  {
    Mat frame;
    cap >> frame; // get a new frame from camera
    /*
    cvtColor(frame, edges, CV_BGR2GRAY);
    GaussianBlur(edges, edges, Size(7,7), 1.5, 1.5);
    Canny(edges, edges, 0, 30, 3);
    imshow("edges", edges);
    */

    cvtColor(frame, edges, CV_BGR2YUV);
    writer << frame;
    imshow("edges", frame);
    if(waitKey(30) >= 0) break;
  }
  // the camera will be deinitialized automatically in VideoCapture destructor
  return 0;
  
}

