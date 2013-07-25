#ifndef _HISTOGRAM_PROCESS_H_
#define _HISTOGRAM_PROCESS_H_

#include <opencv2/opencv.hpp>
#include <opencv/cv.h>
#include <opencv/highgui.h>


extern "C" { 
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavutil/avutil.h"
#include "libswscale/swscale.h"
}

#include "keyframe_utils.h"

/// \class histogram_process  histogram_process.h
/// \brief The class implements all the methods needed to compute and compare histograms
///
class histogram_process{
public:
	/// Copy constructor for this class. 
	/// Utilizes the codec construct to allocate memory for the histograms
	///
	histogram_process(keyframe_utils& u);

	/// Default destructor
	~histogram_process();

	/// \brief Computes histogram for a cv::Mat object and saves it as a cv::MatND object
	/// \param (src) - input matrix
	/// \param (hist) - output hostogram
	/// \return the status of the function.
	/// 
	bool get_histogram(const cv::Mat& src, cv::MatND& hist);
	/// \brief Converts an AVFrame to a cv::Mat and then computes histogram.
	bool get_histogram(const AVFrame *pic, const AVCodecContext *codec, cv::MatND& hist);

	/// \brief - Compares histograms and computes the chi-squared distance
	/// \param(h1, h2) - input histograms
	/// \param(dist) - output parameter, chi-squared distance
	/// \return - status
	bool compare(const cv::MatND& h1, const cv::MatND& h2, double& dist);
	/// \brief - Computes histogram over a picture and compares it to another histogram and calculates distance
	bool compare(const AVFrame *pic1, const cv::MatND& h2, const AVCodecContext *codec, double& dist);
	/// \brief - Computes histograms for both pictures and compares their histograms and calculates distance
	bool compare(const AVFrame *pic1, const AVFrame *pic2, const AVCodecContext *codec, double& dist);
private:
	AVFrame *avFrameRGB;
	struct SwsContext *img_convert_ctx;
	keyframe_utils _util;

};

#endif