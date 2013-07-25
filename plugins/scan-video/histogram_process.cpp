#include "histogram_process.h"
#include "libavutil/avconfig.h"
#include <sys/time.h>
#include <sstream>
#include <fstream>
#include <vector>

using namespace cv;
using namespace std; 

histogram_process::histogram_process(keyframe_utils& u){
	_util = u;
}

histogram_process::~histogram_process(){	
}

bool histogram_process::get_histogram(const cv::Mat& src, cv::MatND& hist){
	try {
		Mat hsv, rgb;
		cvtColor(src, hsv, CV_BGR2HSV);

		// Quantize the hue to 30 levels
		// and the saturation to 32 levels
		//int hbins = 30, sbins = 32;
		int histSize[2] = {30, 32};

		// hue varies from 0 to 179, see cvtColor
		float hranges[2] = { 0, 180 };

		// saturation varies from 0 (black-gray-white) to
		// 255 (pure spectrum color)
		float sranges[2] = { 0, 256 };
		const float* ranges[] = { hranges, sranges };

		// we compute the histogram from the 0-th and 1-st channels
		int channels[2] = {0, 1};

		calcHist( &hsv, 1, channels, Mat(), hist, 2, histSize, ranges,true, false );
		normalize( hist, hist, 1, 0, NORM_L2, -1, Mat() );
	} catch (...) {
		std::cerr << __FILE__ << ":" << __LINE__ 
			<< ":get_histogram fails.\n";
		return false;
	}
	return true;
}

bool histogram_process::get_histogram(const AVFrame *pic, const AVCodecContext *codec, cv::MatND& hist){
	Mat m;
	_util.initialize(codec);
	if(!_util.convert_frame_to_mat(pic, m, codec))	{
	std::cerr << __FILE__ << ":" << __LINE__ 
			<< ":conversting frame to matrix fails.\n";
			return false;
	}
	return get_histogram(m, hist);

}

bool histogram_process::compare(const cv::MatND& h1, const cv::MatND& h2, double& dist){
	try {
		dist = compareHist(h1, h2, CV_COMP_CHISQR);
	} catch (...) {
		std::cerr << __FILE__ << ":" << __LINE__ 
			<< ":comparing histograms fails.\n";
		return false;
	}
	return true;

}

bool histogram_process::compare(const AVFrame *pic1, const cv::MatND& h2, const AVCodecContext *codec, double& dist){

	Mat m1;
	_util.initialize(codec);
	if(!_util.convert_frame_to_mat(pic1, m1, codec)){
	std::cerr << __FILE__ << ":" << __LINE__ 
			<< ":conversting frame to matrix fails.\n";
			return false;
	}

	MatND h1;
	if(!get_histogram(m1, h1)){
		std::cerr << __FILE__ << ":" << __LINE__ 
			<< ":compare fails.\n";
		return false;
	}

	return compare(h1, h2, dist);

}

bool histogram_process::compare(const AVFrame *pic1, const AVFrame *pic2, const AVCodecContext *codec, double& dist){

	Mat m1;
	_util.initialize(codec);
	if(!_util.convert_frame_to_mat(pic1, m1, codec))
		return false;
	MatND h1;
	if(!get_histogram(m1, h1))
		return false;
	return compare(pic2, h1, codec, dist);
}