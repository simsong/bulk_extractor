#ifndef _EXTRACT_KEYFRAME_H_
#define _EXTRACT_KEYFRAME_H_


#include "../src/bulk_extractor.h"
#include "histogram_process.h"

extern "C" {
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavutil/avutil.h"
#include "libswscale/swscale.h" 
}
#include <cstring>

/// \class shot_segment extract_keyframes.h
/// \brief The class represents the boundaries of a video segment "shot"
///

class shot_segment{
public:
	// default constructor
	shot_segment();
	// default destructor
	~shot_segment();

	// the starting and ending histograms
	cv::MatND _start_hist;
	cv::MatND _end_hist;

	// the starting and ending presentation times (AVFrame::pts defined in avcodec/avcodec.h)
	uint64_t _start_pos;		
	uint64_t _end_pos;

	// the starting and ending frame numbers.
	uint64_t _start_frm;		
	uint64_t _end_frm;
};

/// \class extract_keyframes  extract_keyframes.h
/// \brief The class implements all the methods needed to extract keyframes and identify scene changes.
///

class extract_keyframes {
public:

	// default constructor
	extract_keyframes();

	// default destructor
	~extract_keyframes();

	/// \brief The main worker function.
	/// \param (input_file) The input video file from which needs to be processed
	/// \return the status of the function.
	/// 
	/// This function extracts key frames and identifies scene changes for the given
	/// input file. It initializes the ffmpeg constructs and also frees them accordingly.
	/// 
	bool extract(std::string input_file);

	/// If the input file is in memory (sbuf, offset) then this function
	/// saves the output file to the output folder (specified by the recorder)
	/// and then invokes the overloaded function extract(std::string), to
	/// extract key frames.
	bool extract(const sbuf_t& sbuf, size_t offset, feature_recorder& recorder);

private:

	/// \brief Decodes the compressed bitstream packet to AVFrame
	/// \param (format_context) FFMPEG video decoder constructs (libavformat/avformat.h)
	/// \param (codec_context) FFMPEG video decoder constructs  (libavcodec/avcodec.h)
	/// \param (video_stream) Video decoder handle.
	/// \param (frame) Decoded output frame
	/// \return the status of the function.
	/// 
	bool next_frame(AVFormatContext *format_context, AVCodecContext *codec_context, int video_stream, AVFrame *frame);
	
	/// \brief Segments the video, calculates the motion score for each frame. extracts key frames and identifies scene changes.
	/// \param (format_context) FFMPEG video decoder constructs (libavformat/avformat.h)
	/// \param (codec_context) FFMPEG video decoder constructs  (libavcodec/avcodec.h)
	/// \param (video_stream) Video decoder handle.
	/// \return the status of the function.
	/// 
	bool segmentation(AVFormatContext *format_context, AVCodecContext *codec_context, int video_stream);

	/// \brief Initializes the key frame utilities and the histogram process methods.
	/// \param (codec_context) FFMPEG video decoder constructs  (libavcodec/avcodec.h)
	///
	void initialize(const AVCodecContext *codec_context);

	/// \brief Creates the output directory as specified by the input file
	/// The extracted keyframes & the scene_change.txt file will be saved
	/// in this folder.
	/// \param (name) The name of the input file, from which the directory name is 
	/// generated
	/// \return the status of the function
	///
	bool create_outdir(const std::string name);
	
	// Instance of the histogram process interface. 
	histogram_process* _hp;
	// Instance of the keyframe utilities. 
	keyframe_utils _util;

	//Instance of FFMPEG video decoder constructs - AVPacket (libavcodec/avcodec.h)
	AVPacket _packet;
	// Tracks the initialization process for the AVPacket
	bool _firsttime;

	//output directory name.
	std::string _dir;

};

#endif