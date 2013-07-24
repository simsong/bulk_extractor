#ifndef _KEYFRAME_UTILS_H_
#define _KEYFRAME_UTILS_H_

#include <opencv2/opencv.hpp>
#include <opencv/cv.h>
#include <opencv/highgui.h>


extern "C" {
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavutil/avutil.h"
#include "libswscale/swscale.h"
}
#include <math.h>

/// \class keyframe_utils keyframe_utils.h
/// \brief The class implements utilities needed by the extract_keyframes class 
/// and the histogram_process class.
///

class keyframe_utils{
public:
	
	/// Default constructor
	keyframe_utils();

	/// Copy constructor - uses the FFMPEG codec construct to allocate memory
	/// which will be used by the histogram process. 
	keyframe_utils(const keyframe_utils& u);

	/// Default Destructor
	~keyframe_utils();
	
	/// Assignment operator
	keyframe_utils& operator= (const keyframe_utils& u);
	
	/// \brief Interfaces FFMPEG data structures (AVFrame) to OpenCV data structures (cv::Mat)
	/// \param (src) - input frame - AVFrame  (libavcodec/avcodec.h)
	/// \param (dst) - output matrix 
	/// \param (codec) FFMPEG video decoder constructs  (libavcodec/avcodec.h)
	/// \return the status of the function.
	/// 
	bool convert_frame_to_mat(const AVFrame *src, cv::Mat& dst, const AVCodecContext *codec);
	
	/// \brief Saves the AVFrame to disk as a jpeg file
	/// \param (src) - input frame - AVFrame  (libavcodec/avcodec.h)
	/// \param (file_name) - name & path of the output jpeg file
	/// \param (codec) FFMPEG video decoder constructs  (libavcodec/avcodec.h)
	/// \return the status of the function.
	/// 	
	bool save_keyframe(const std::string file_name, const AVCodecContext *codec, const AVFrame *src);
	
	/// \brief - Initializes all the parameters based on the codec parameters.
	bool initialize(const AVCodecContext *codec);
	
	/// \brief - resets all the parameters to default values
	bool reset();

	/// \brief - Calulates the motion_score and intra_likelihood
	/// \param (f) - input frame - AVFrame  (libavcodec/avcodec.h)
	/// \param (codec) FFMPEG video decoder constructs  (libavcodec/avcodec.h)
	/// \param(score) - motion score for the given input frame
	/// \param(intra_likelihood) - ratio of intra macroblock to the frame size
	/// \return the status of the function.
	/// 	
	bool calculate_motion_score(const AVFrame *f, const AVCodecContext *codec, double& score, double &intra_likelihood);

	/// The below utility functions identify the macro block type
	static inline bool IS_INTERLACED(int a) { if((a) & MB_TYPE_INTERLACED) return true; return false; }

	static inline bool IS_16X16(int a) { if((a) & MB_TYPE_16x16) return true; return false; }

	static inline bool IS_16X8(int a) { if((a) & MB_TYPE_16x8) return true; return false; }

	static inline bool IS_8X16(int a) { if((a) & MB_TYPE_8x16) return true; return false; }

	static inline bool IS_8X8(int a) { if((a) & MB_TYPE_8x8) return true; return false; }

	static inline bool USES_LIST(int a, int list){ if ((a) & ((MB_TYPE_P0L0|MB_TYPE_P1L0)<<(2*(list)))) return true; return false; }

	static inline bool IS_INTRA4x4(int a) { if((a) & MB_TYPE_INTRA4x4) return true; return false; }

	static inline bool IS_INTRA16x16(int a) { if((a) & MB_TYPE_INTRA16x16) return true; return false; }

	static inline bool IS_IPCM(int a) { if((a) & MB_TYPE_INTRA_PCM) return true; return false; }

	static inline bool IS_INTRA(int a) { if((a) & 7) return true; return false; }

	static inline bool IS_INTER(int a) { if((a) & (MB_TYPE_16x16|MB_TYPE_16x8|MB_TYPE_8x16|MB_TYPE_8x8)) return true; return false; }

	static inline bool IS_SKIP(int a) { if((a) & MB_TYPE_SKIP) return true; return false; }

	static inline float get_magnitude(int x, int y){ return sqrt((x * x) + (y * y)); } 


private:
	AVFrame *av_frame_rgb;				/// FFMPEG construct - libavcodec/avcodec.h
	struct SwsContext *img_convert_ctx;	/// FFMPEG construct - libswscale/swscale.h
	bool is_initialzed;

};

#endif