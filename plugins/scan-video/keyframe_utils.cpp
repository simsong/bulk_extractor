#include "keyframe_utils.h"
#include <fstream>

using namespace cv;
using namespace std;

keyframe_utils::keyframe_utils(): av_frame_rgb(NULL), img_convert_ctx(NULL), is_initialzed(false){}

keyframe_utils::~keyframe_utils(){}

keyframe_utils::keyframe_utils(const keyframe_utils& u): av_frame_rgb(u.av_frame_rgb), img_convert_ctx(u.img_convert_ctx), is_initialzed(u.is_initialzed){}

keyframe_utils& keyframe_utils::operator= (const keyframe_utils& u){
	av_frame_rgb = u.av_frame_rgb;
	img_convert_ctx = u.img_convert_ctx;
	is_initialzed = u.is_initialzed;
	return *this;
}


bool keyframe_utils::initialize(const AVCodecContext *codec){
	if(is_initialzed) 
		return true;
	is_initialzed = true;
	unsigned int h = codec->height;
	unsigned int w = codec->width;
	try {
		/// Initialze SwsContext to convert compressed videos from native format (YUV420, YVY12 etc) 
		/// to BGR24 using bicubic interpolation
		if(img_convert_ctx == NULL) {
			img_convert_ctx = sws_getContext(w, h, codec->pix_fmt, w, h, PIX_FMT_BGR24, SWS_BICUBIC, NULL, NULL, NULL);
			if(img_convert_ctx == NULL) {
				std::cerr << __FILE__ << " : " << __LINE__ << " Cannot initialize the conversion context. w = "<< w << " h = " << h  << endl;
				return false;
			}
		}
		//  Allocate memory for a BGR 24 frame of resolution w x h
		av_frame_rgb = avcodec_alloc_frame();
		if (!av_frame_rgb){
			std::cerr << __FILE__ << " : " << __LINE__ << "Unable to allocate frame memory\n";
			return false;
		}

		uint8_t *picture_buf;
		int size = avpicture_get_size(PIX_FMT_BGR24, w, h);  
		picture_buf = (uint8_t *) av_malloc(size);
		if (!picture_buf) {
			std::cerr << __FILE__ << " : " << __LINE__ << "Unable to allocate frame memory\n";
			av_free(av_frame_rgb);
			return false;
		}
		avpicture_fill((AVPicture *)av_frame_rgb, picture_buf,PIX_FMT_BGR24, w, h);
	}catch (...) {
		std::cerr << __FILE__ << ":" << __LINE__ 
			<< ": Unable to create cv::Mat object.\n";
		return false;
	}
	return true; 
}

bool keyframe_utils::reset(){
	sws_freeContext(img_convert_ctx); 
	av_free(av_frame_rgb);
	is_initialzed = false;
	return true;	
}

bool keyframe_utils::convert_frame_to_mat(const AVFrame *src, cv::Mat& dst, const AVCodecContext *codec){
	unsigned int h = codec->height;
	unsigned int w = codec->width;

	try{
		sws_scale(img_convert_ctx, src->data, src->linesize, 0, h, av_frame_rgb->data, av_frame_rgb->linesize);
		unsigned char* data = av_frame_rgb->data[0];
		unsigned int data_size = av_frame_rgb->linesize[0];
		dst.create(h, w, CV_8UC3); 
		for(size_t y = 0; y < h; ++y){
			for (size_t x = 0; x < w; ++x){
				dst.at<cv::Vec3b>(y,x)[0] = data[y * data_size + x * 3 + 0];
				dst.at<cv::Vec3b>(y,x)[1] = data[y * data_size + x * 3 + 1];
				dst.at<cv::Vec3b>(y,x)[2] = data[y * data_size + x * 3 + 2];
			}
		} 
	} catch (...) {
		std::cerr << __FILE__ << ":" << __LINE__ 
			<< ": Unable to create cv::Mat object.\n";
		return false;
	}
	return true;
}

bool keyframe_utils::save_keyframe(const std::string file_name, const AVCodecContext *codec, const AVFrame *src){
	try {
		Mat dst;
		if(!convert_frame_to_mat(src, dst, codec))
			return false;
		imwrite(file_name.c_str(), dst);

	} catch (...) {
		std::cerr << __FILE__ << ":" << __LINE__ 
			<< ": Unable to save key frame.\n";
		return false;
	}
	return true;
}

bool keyframe_utils::calculate_motion_score(const AVFrame *f, const AVCodecContext *codec, double& score, double &intra_likelihood){
	
	// Calculate MB stride and set parameters to use sub-pel Motion vectors
	int mb_width  = (codec->width + 15) / 16;
	int mb_height = (codec->height + 15) / 16;
	int mb_stride = mb_width + 1;
	int mv_sample_log2 = 4 - f->motion_subsample_log2;
	int mv_stride = (mb_width << mv_sample_log2) + (codec->codec_id == CODEC_ID_H264 ? 0 : 1);
	int quarter_sample = (codec->flags & CODEC_FLAG_QPEL) != 0;
	int shift = 1 + quarter_sample;
	int direction = 0;
	double intra_count = 0.0;
	
	if ((NULL == f->motion_val) || (NULL == f->motion_val[direction]) || (NULL == f->motion_val[direction][0])){
		cerr << "MV Array is NULL\n";
		return false;
	}
	
	int mb_index = 0;
	int xy = 0, dx = 0, dy = 0; 
	score = 0; intra_likelihood = 0;

	
	// Iterate through all the Macroblocks in the frame and check if -
	// * Is it intra or not?
	// * If it has a motion vector, get magnitude
	// Calculate final scores
	for (int mb_y = 0; mb_y < mb_height; ++mb_y) {
		for (int mb_x = 0; mb_x < mb_width; ++mb_x) {
			mb_index = mb_x + mb_y * mb_stride;		

			if (!USES_LIST(f->mb_type[mb_index], direction)) {
				if (IS_8X8(f->mb_type[mb_index])) {
					intra_count += 0.25;
				} else if ((IS_16X8(f->mb_type[mb_index])) || (IS_8X16(f->mb_type[mb_index]))){
					intra_count += 0.5;
				}else if((IS_INTRA(f->mb_type[mb_index])) || (IS_IPCM(f->mb_type[mb_index])) || (IS_INTRA16x16(f->mb_type[mb_index]))){
					intra_count += 1.0;
				}else {
					intra_count += 1.0;
				}
				continue;
			}

			if (IS_8X8(f->mb_type[mb_index])) {
				for (int i = 0; i < 4; i++) {
					xy = (mb_x*2 + (i&1) + (mb_y*2 + (i>>1))*mv_stride) << (mv_sample_log2-1);
					dx = (f->motion_val[direction][xy][0]>>shift);
					dy = (f->motion_val[direction][xy][1]>>shift);
					
					score += get_magnitude(dx, dy); 
				}
			} else if (IS_16X8(f->mb_type[mb_index])) {
				for (int i = 0; i < 2; i++) {
					xy = (mb_x*2 + (mb_y*2 + i)*mv_stride) << (mv_sample_log2-1);
					dx = (f->motion_val[direction][xy][0]>>shift);
					dy = (f->motion_val[direction][xy][1]>>shift);

					if (IS_INTERLACED(f->mb_type[mb_index]))
						dy *= 2;
					
					score += get_magnitude(dx, dy);
				}
			} else if (IS_8X16(f->mb_type[mb_index])) {
				for (int i = 0; i < 2; i++) {
					xy =  (mb_x*2 + i + mb_y*2*mv_stride) << (mv_sample_log2-1);
					dx = (f->motion_val[direction][xy][0]>>shift);
					dy = (f->motion_val[direction][xy][1]>>shift);

					if (IS_INTERLACED(f->mb_type[mb_index]))
						dy *= 2;
					
					score += get_magnitude(dx, dy);
				}
			} else {
				xy = (mb_x + mb_y*mv_stride) << mv_sample_log2;
				dx = (f->motion_val[direction][xy][0]>>shift);
				dy = (f->motion_val[direction][xy][1]>>shift);
					
				score += get_magnitude(dx, dy);
			}

		}

		intra_likelihood = intra_count / (mb_height * mb_width * 1.0);
		score = score / (mb_height * mb_width * 1.0);
	}
	return true;
}