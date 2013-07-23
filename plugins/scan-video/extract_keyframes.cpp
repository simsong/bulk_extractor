#include "extract_keyframes.h"
#include "video_processor.h"

#include <iostream>
#include <fstream>
#include <exception>

#include <opencv2/opencv.hpp>
#include <opencv/cv.h>
#include <opencv/highgui.h>

// Install Signal Actions
#include <unistd.h>
#include <sys/types.h>
#ifndef WIN32
#include <sys/wait.h>
#endif
#include <signal.h>

using namespace std;

shot_segment::shot_segment():_start_pos(0), _end_pos(0), _start_frm(0), _end_frm(0){}

shot_segment::~shot_segment(){}

extract_keyframes::extract_keyframes():_firsttime(true), _hp(NULL){
	_util.reset();
	_dir.clear();
	av_init_packet(&_packet);
}

extract_keyframes::~extract_keyframes(){
	if(NULL != _hp){
		delete _hp;
		_hp = NULL;
	}
	_util.reset();
}

void extract_keyframes::initialize(const AVCodecContext *codec_context){

	_util.reset();
	_util.initialize(codec_context);
	av_init_packet(&_packet);

	if(NULL != _hp){
		delete _hp;
		_hp = NULL;
	}
	_hp = new histogram_process(_util);
	return;
}

bool extract_keyframes::next_frame(AVFormatContext *format_context, AVCodecContext *codec_context,
	int videoStream, AVFrame *frame){

		int             bytesDecoded;
		int             frameFinished;		// returns function status
		if (_firsttime) {
			_firsttime = false;
			_packet.data = NULL;
			_packet.size = 0;
		}

		while (true) {
			// Decode the packet to frame. Use as many packets until a single frame is decoded.
			while (_packet.size > 0) {
				bytesDecoded = avcodec_decode_video2(codec_context, frame, &frameFinished, &_packet);
				if (bytesDecoded < 0) {
					std::cerr << __FILE__ << " : " << __LINE__ << 
						" Error while decoding frame\n";
					return false;
				}
				_packet.size -= bytesDecoded;
				_packet.data += bytesDecoded;
				// If a complete frame has been decoded, return.
				if (frameFinished)
					return true;
			}
			do {
				// read packets.
				if (av_read_frame(format_context, &_packet) < 0){
					bytesDecoded = avcodec_decode_video2(codec_context, frame, &frameFinished, &_packet);
					return frameFinished != 0;
				}
			} while (_packet.stream_index != videoStream);
		}

		// decode last packet to frame
		bytesDecoded = avcodec_decode_video2(codec_context, frame, &frameFinished, &_packet);
		return frameFinished != 0;
}

bool extract_keyframes::segmentation(AVFormatContext *format_context, AVCodecContext *codec_context, int video_handle){

	// Allocate video frame
	AVFrame *frame = avcodec_alloc_frame();
	// Allocate histograms to track -
	// i_frame - Previous I frame for scene change comparison
	// key_frame - Previous key frame for key frame extraction
	// curr - Holds the histogram for the current frame.
	cv::MatND i_frame, key_frame, curr;

	// Holds the shot segment information for scene change detection.
	shot_segment sc;

	// first time - get frame and calc histogram. For each I frame note down the byte position
	if(next_frame(format_context, codec_context, video_handle, frame)){
		_hp->get_histogram(frame, codec_context, i_frame);
		key_frame = i_frame.clone();		// use first frame as key reference. although this frame is not saved.
		curr = i_frame.clone();		// use first frame as key reference. although this frame is not saved.
		sc._end_pos = 0; sc._end_frm = 0;

	}else {
		std::cerr << __FILE__ << " : " << __LINE__ << 
			" Error decoding the video stream\n";
		av_free(frame);
		return false;
	}


	double motion_score = 0, intra_likelihood = 0;
	double dist_intra = 0.0, dist_key = 0.0;
	bool do_only_hist = false, save_first = false;
	int picture_number = 0;

	double frame_rate = 1.0;
	if((codec_context->time_base.num != 0) && (codec_context->time_base.den != 0))
		frame_rate = 1.0 * codec_context->time_base.den / codec_context->time_base.num;

	// Scene Change Output
	std::string sc_file = _dir + "/" + "scene_change.txt";
	std::ofstream fp_sc(sc_file.c_str(),  ios::binary | ios::out);
	if(NULL == fp_sc){
		std::cerr << __FILE__ << " : " << __LINE__ << 
			" Unable to create scene change file\n";
		return false;
	}

	// Decode until last frame
	while  (next_frame(format_context, codec_context, video_handle, frame)) {

		//shot segmentation
		++picture_number;	
		// Process I-Frames
		if((frame->key_frame) || (frame->pict_type == AV_PICTURE_TYPE_I) || (do_only_hist)){

			// Save first I frame as a key frame. This is essential for frames with a very low motion score. 
			// If none of the frames have the required motion activity then no frame will be saved
			// To prevent that we save the first I frame as a key frame.
			if(!save_first){
				// Copy histograms for the next keyframe and scene change calculation 
				key_frame = curr.clone();
				i_frame = curr.clone();
				// Generate the key frame name
				stringstream name;
				name  <<"key_" << picture_number << ".jpg";
				std::string kf = _dir + "/" + name.str();
				if(!_util.save_keyframe(kf, codec_context, frame)){
					std::cerr << __FILE__ << " : " << __LINE__ << 
						" Error saving key frame " << kf << std::endl;
					fp_sc.close();
					av_free(frame);
					return false;
				}
				save_first = true;
				continue;
			}

			// Copy histograms & shot boundaries for the next keyframe and scene change calculation 
			sc._start_pos = sc._end_pos; sc._start_frm = sc._end_frm;
			sc._end_pos = frame->pts;
			sc._end_frm = frame->coded_picture_number;
			// if picture numbers are not coded, use local count
			if(0 == frame->coded_picture_number) 
				sc._end_frm = picture_number;
			//Calculate histogram for the current frame
			_hp->get_histogram(frame, codec_context, curr);

			// compute distance to the previous intra frame histogram
			if(!_hp->compare(curr, i_frame, dist_intra)){
				std::cerr << __FILE__ << " : " << __LINE__ << 
					" Error calculating histogram distance" << std::endl;
				fp_sc.close();
				av_free(frame);
				return false;
			}
			i_frame = curr.clone();

			if(dist_intra > 0.91){		//THRESHOLD_SHOT_SEG = 0.91
				// Output to scene_change.txt
				if(sc._start_pos < 0)
					fp_sc << "Timestamp :  NULL - NULL | Frame Number : " << sc._start_frm <<" - " << sc._end_frm << endl;
				else
					fp_sc << "Timestamp :  " << (1.0 * sc._start_pos / frame_rate)  <<" - "  <<  (1.0 * sc._end_pos / frame_rate) <<  " | Frame Number : " << sc._start_frm <<" - " << sc._end_frm << endl;

				// compute distance to previous key frame histogram 
				if(!_hp->compare(curr, key_frame, dist_key)){
					std::cerr << __FILE__ << " : " << __LINE__ << 
						" Error calculating histogram distance"  << std::endl;
					fp_sc.close();
					av_free(frame);
					return false;
				}

				if(dist_key > 0.26){	//THRESHOLD_SHOT_KEY = 0.26
					//Current frame is also a key frame; Save frame
					key_frame = curr.clone();
					stringstream name;
					name  <<"key_" << picture_number << ".jpg";
					std::string kf = _dir + "/" + name.str();
					if(!_util.save_keyframe(kf, codec_context, frame)){
						std::cerr << __FILE__ << " : " << __LINE__ << 
							" Error saving frames" << kf << std::endl;
						fp_sc.close();
						av_free(frame);
						return false;
					}
				}
			}
		}else {
			// Process P-frames and B-frames
			// Calculate motion score and intra likelihood
			if(!_util.calculate_motion_score(frame, codec_context, motion_score, intra_likelihood)){
				// error computing the motion score. Use only the histogram method
				do_only_hist = true;
				continue;
			}
			// Check thresholds for THRESHOLD_AVG_MOTION_SCORE & THRESHOLD_INTRA_LIKELIHOOD
			if((motion_score > 2.57) || (intra_likelihood > 0.4)){
				_hp->get_histogram(frame, codec_context, curr);
				// Compute distances between the current frame and the previous I frame and Key frame.
				if(!_hp->compare(curr, i_frame, dist_intra)){
					std::cerr << __FILE__ << " : " << __LINE__ << 
						" Error calculating histogram distance" << std::endl;
					fp_sc.close();
					av_free(frame);
					return false;
				}

				if(!_hp->compare(curr, key_frame, dist_key)){
					std::cerr << __FILE__ << " : " << __LINE__ << 
						" Error calculating histogram distance" << std::endl;
					fp_sc.close();
					av_free(frame);
					return false;
				}

				if((dist_key >  0.26) && (dist_intra > 0.9)){
					sc._start_pos = sc._end_pos; sc._start_frm = sc._end_frm;
					sc._end_pos = frame->pts;
					sc._end_frm = frame->coded_picture_number;
					if(0 == frame->coded_picture_number) sc._end_frm = picture_number;
					if(sc._start_pos < 0)
						fp_sc << "Timestamp :  NULL - NULL | Frame Number : " << sc._start_frm <<" - " << sc._end_frm << endl;
					else
						fp_sc << "Timestamp :  " << (1.0 * sc._start_pos / frame_rate)  <<" - "  <<  (1.0 * sc._end_pos / frame_rate) <<  " | Frame Number : " << sc._start_frm <<" - " << sc._end_frm << endl;

					key_frame = curr.clone();

					stringstream name;
					name  <<"key_" << picture_number << ".jpg";
					std::string kf = _dir + "/" + name.str();
					if(!_util.save_keyframe(kf, codec_context, frame)){
						std::cerr << __FILE__ << " : " << __LINE__ << 
							" Error saving frames"  << std::endl;
						fp_sc.close();
						av_free(frame);
						return false;

					}
				}
			}
		}
	}

	fp_sc.close();
	av_free(frame);
	return true;
}

bool extract_keyframes::create_outdir(const std::string name){
	//remove all dots
	_dir.assign(name);
	if(_dir.size() == 0) {
		std::cerr << __FILE__ << " : " << __LINE__ << " Invalid directory name\n";
		return false;
	}
	// Generate Directory name, by replacing all dots with '_'
	size_t position = name.find_last_of('.');
	if(position != std::string::npos){
		_dir.replace(position, 1, "_");
	}
#ifdef WIN32
	if(mkdir(_dir.c_str())){
		std::cerr << __FILE__ << " : " << __LINE__ <<  " Could not make directory " << _dir << "\n";
		return false;
	}
#else
	if(mkdir(_dir.c_str(),0777)){
		std::cerr << __FILE__ << " : " << __LINE__ <<  " Could not make directory " << _dir << "\n";
		return false;
	}
#endif
	return true;
}

bool extract_keyframes::extract(const sbuf_t& sbuf, size_t offset, feature_recorder& recorder){

	char* data = (char*) (sbuf.buf + offset);
	size_t l = sbuf.pagesize;
	size_t length = l - offset;
	const pos0_t &pos0 = sbuf.pos0;
	std::string fpath = pos0.path;
	// Get the memory buffer pointer and length of buffer
	if (! data || length == 0) {
		std::cerr << __FILE__ << " : " << __LINE__ <<  " Invalid argument to extract_keyframes::extract" << std::endl;
		return false;
	}
	// construct output file name
	std::stringstream ss;
	ss << recorder.name << "-p" << fpath << "-" << pos0.offset
		<< "-" << offset << "-" << rand() << "." << recorder.name;
	std::string outfile = recorder.outdir + "/" + ss.str();
	std::fstream fs(outfile.c_str(), std::ios_base::out | std::ios_base::binary);
	fs.write(data, length);
	bool ok = ! fs.bad();
	if (! ok) {
		std::cerr << __FILE__ << " : " << __LINE__ <<  " >> ERROR writing buffer to output file: " << outfile << std::endl;
	}
	fs.close();
	ok &= extract(outfile);
	return ok;
}

bool extract_keyframes::extract(std::string input_file){

	if(input_file.empty()){
		std::cerr << __FILE__ << " : " << __LINE__ << 
			" Invalid input file name\n";
		return false;
	}
	bool ok = true;
#ifndef WIN32
	pid_t pid = fork();
	if (pid == 0) {
		install_signal_actions();

		try {
			if(!create_outdir(input_file)){
				std::cerr << __FILE__ << " : " << __LINE__ << 
					" Creation of Output Directory failed\n";
				throw new std::exception();
			}

			// Initialize all the ffmpeg constructs - AVCodec, AVFormat, AVPacket.
			av_register_all();
			avcodec_register_all();
			av_init_packet(&_packet);

			// Open the input file
			AVFormatContext *format_context = NULL;
			if (avformat_open_input(&format_context, input_file.c_str(), NULL, NULL) != 0){
				std::cerr << __FILE__ << " : " << __LINE__ << 
					" Failed to open the input file\n";
				throw new std::exception();
			}

			// Retrieve stream information
			if (avformat_find_stream_info(format_context,NULL) < 0){
				std::cerr << __FILE__ << " : " << __LINE__ << 
					" Failed find stream parameters in the input file\n";
				throw new std::exception();
			}

			// Identify the video handle
			int video_handle = -1;
			AVCodecContext *temp_context = NULL;
			for (int i = 0; i < format_context->nb_streams; i++) {
				temp_context = format_context->streams[i]->codec;
				if (temp_context->codec_type == AVMEDIA_TYPE_VIDEO) {
					video_handle = i;
					break;
				}
			}
			if (video_handle == -1){
				std::cerr << __FILE__ << " : " << __LINE__ << 
					" No valid video handle identified in the input file\n";
				throw new std::exception();
			}

			// Get a pointer to the codec context for the video stream
			AVCodecContext *codec_context = format_context->streams[video_handle]->codec;
			// Find the decoder for the video stream
			AVCodec *codec = avcodec_find_decoder(codec_context->codec_id);
			if (codec == NULL){
				std::cerr << __FILE__ << " : " << __LINE__ << 
					" Invalid video parameters in the input file\n";
				throw new std::exception();
			}

			// Inform the codec that we can handle truncated bitstreams
			if (codec->capabilities & CODEC_CAP_TRUNCATED)
				codec_context->flags |= CODEC_FLAG_TRUNCATED;

			// Initialize the identified video codec
			if (avcodec_open2(codec_context, codec, NULL) < 0){
				std::cerr << __FILE__ << " : " << __LINE__ << 
					" Failed to initialize the video decoder\n";
				throw new std::exception();
			}

			// Based on the identified codec - initialize the histogram process
			// and keyframe utilities.
			initialize(codec_context);

			// Segment the video based on the motion score and identify
			// scene changes. Process the segmented video and extract
			// key frames.
			ok = segmentation(format_context, codec_context, video_handle);

			// release the ffmpeg constructs.
			avcodec_close(codec_context);
			avformat_close_input(&format_context);
		} catch (std::exception& e) {
			std::cerr << __FILE__ << " : " << __LINE__<< " extract key frames failed: " << e.what() << std::endl;
			exit(1);
		}
		exit(0);
	}  else {
		//std::cerr << "ExtractKeyframes >> Waiting for pid " << pid << std::endl;
		int stat;
		pid_t child_pid = waitpid(pid, &stat, 0);
		if (child_pid == -1) {
			std::cerr << __FILE__ << ":" << __LINE__ <<" ExtractKeyframes >> Child process " << pid << " failed." << std::endl;
			return false;
		} else if (stat != 0) {
			std::cerr << __FILE__ << ":" << __LINE__ <<" ExtractKeyframes >> Child process " << pid << " finished with error "
				<< (stat & 0xff00 >> 8)
				<< "," << (stat & 0xff) << std::endl;
		} else {
			//std::cerr << "ExtractKeyframes >> Child process " << pid << " succeeded." << std::endl;
		}
	}
#else

	try {
		if(!create_outdir(input_file)){
			std::cerr << __FILE__ << " : " << __LINE__ << 
				" Creation of Output Directory failed\n";
			throw new std::exception();
		}

		// Initialize all the ffmpeg constructs - AVCodec, AVFormat, AVPacket.
		av_register_all();
		avcodec_register_all();
		av_init_packet(&_packet);

		// Open the input file
		AVFormatContext *format_context = NULL;
		if (avformat_open_input(&format_context, input_file.c_str(), NULL, NULL) != 0){
			std::cerr << __FILE__ << " : " << __LINE__ << 
				" Failed to open the input file\n";
			throw new std::exception();
		}

		// Retrieve stream information
		if (avformat_find_stream_info(format_context,NULL) < 0){
			std::cerr << __FILE__ << " : " << __LINE__ << 
				" Failed find stream parameters in the input file\n";
			throw new std::exception();
		}

		// Identify the video handle
		int video_handle = -1;
		AVCodecContext *temp_context = NULL;
		for (int i = 0; i < format_context->nb_streams; i++) {
			temp_context = format_context->streams[i]->codec;
			if (temp_context->codec_type == AVMEDIA_TYPE_VIDEO) {
				video_handle = i;
				break;
			}
		}
		if (video_handle == -1){
			std::cerr << __FILE__ << " : " << __LINE__ << 
				" No valid video handle identified in the input file\n";
			throw new std::exception();
		}

		// Get a pointer to the codec context for the video stream
		AVCodecContext *codec_context = format_context->streams[video_handle]->codec;
		// Find the decoder for the video stream
		AVCodec *codec = avcodec_find_decoder(codec_context->codec_id);
		if (codec == NULL){
			std::cerr << __FILE__ << " : " << __LINE__ << 
				" Invalid video parameters in the input file\n";
			throw new std::exception();
		}

		// Inform the codec that we can handle truncated bitstreams
		if (codec->capabilities & CODEC_CAP_TRUNCATED)
			codec_context->flags |= CODEC_FLAG_TRUNCATED;

		// Initialize the identified video codec
		if (avcodec_open2(codec_context, codec, NULL) < 0){
			std::cerr << __FILE__ << " : " << __LINE__ << 
				" Failed to initialize the video decoder\n";
			throw new std::exception();
		}

		// Based on the identified codec - initialize the histogram process
		// and keyframe utilities.
		initialize(codec_context);

		// Segment the video based on the motion score and identify
		// scene changes. Process the segmented video and extract
		// key frames.
		ok = segmentation(format_context, codec_context, video_handle);

		// release the ffmpeg constructs.
		avcodec_close(codec_context);
		avformat_close_input(&format_context);
	} catch (std::exception& e) {
		std::cerr << __FILE__ << " : " << __LINE__<< " extract key frames failed: " << e.what() << std::endl;
		exit(1);
	}
#endif
	return ok;
}
