#ifndef _VIDEO_PROCESSOR_H_
#define _VIDEO_PROCESSOR_H_

#include <opencv2/opencv.hpp>

#include "../src/bulk_extractor.h"

#ifndef WIN32
void sig_action(int signal, siginfo_t *si, void *arg);
void install_signal_actions();
#endif

class video_info {
  public:
    int width;
    int height;
    int fps;
    int codec;
    double frame_count;

    video_info();
    ~video_info();

    void init();
    void copy(video_info& dst);
    char* codec_str();
    std::string str();
};

class video_processor {
  public:
    static const int DEFAULT_CODEC;
    static const int CODEC_MJPG;
    static const int CODEC_YUV;
    static const int CODEC_MP4;
    static const int CODEC_H264;
    static const int CODEC_XVID;

    static inline int read_int(const unsigned char* s) {
      if (! s) { return 0; }
      return s[0] << 24 | s[1] << 16 | s[2] << 8 | s[3];
    };

    video_processor();

    bool process_header(const sbuf_t& sbuf, size_t offset, 
                        feature_recorder& recorder);

    bool get_info(const std::string& infile, video_info& info);

    bool convert(const std::string& infile, const std::string& outfile, 
                 video_info& info, int codec = DEFAULT_CODEC);

    bool convert(const char* data, size_t length, const std::string& outfile, 
                 video_info& info, int codec = DEFAULT_CODEC);
	void set_file_name(const std::string &o) { _outfile.assign(o);}
	const std::string& get_file_name() {return _outfile;}
			
  protected:
	  std::string _outfile;

};

#endif
