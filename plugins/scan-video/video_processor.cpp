#include <iostream>
#include <fstream>
#include <exception>
#include <opencv2/opencv.hpp>

#include <unistd.h>
#include <sys/types.h>
#include <signal.h>

#ifndef WIN32
#include <sys/wait.h>
#include <sys/shm.h>
#endif

#include "video_processor.h"

#ifndef WIN32
void sig_action(int signal, siginfo_t *si, void *arg)
{
	//std::cerr << "Caught segfault at address " << si->si_addr << std::endl;
	exit(2);
}

void install_signal_actions() {
	struct sigaction sa;

	memset(&sa, 0, sizeof(struct sigaction));
	sigemptyset(&sa.sa_mask);
	sa.sa_sigaction = sig_action;
	sa.sa_flags = SA_SIGINFO;

	sigaction(SIGSEGV, &sa, NULL);
	sigaction(SIGABRT, &sa, NULL);
	sigaction(SIGFPE, &sa, NULL);
	sigaction(SIGILL, &sa, NULL);
	sigaction(SIGQUIT, &sa, NULL);
	sigaction(SIGTRAP, &sa, NULL);
	sigaction(SIGBUS, &sa, NULL);
	sigaction(SIGXCPU, &sa, NULL);
	sigaction(SIGXFSZ, &sa, NULL);
}
#endif

video_info::video_info():width(0),height(0),fps(1),codec(-1),frame_count(0)
{
}

video_info::~video_info()
{
}

void video_info::init()
{
  width = 0;
  height = 0;
  fps = 1;
  codec = -1;
  frame_count = 0;
}

char* video_info::codec_str() 
{
	char* buf = new char[5];
	buf[0] = codec & 0XFF;
	buf[1] = (codec & 0XFF00) >> 8;
	buf[2] = (codec & 0XFF0000) >> 16;
	buf[3] = (codec & 0XFF000000) >> 24;
	buf[4] = 0;
	return buf;
}

std::string video_info::str()
{
	std::stringstream s; 
	s << "width=" << width << ",height=" << height 
		<< ",fps=" << fps << ",codec=" << codec_str()
		<< ",frame_count=" << frame_count;
	return s.str();
}

void video_info::copy(video_info& dst)
{
  dst.width = width;
  dst.height = height;
  dst.fps = fps;
  dst.codec = codec;
  dst.frame_count = frame_count;
}

const int video_processor::CODEC_MJPG = CV_FOURCC('M','J','P','G');
const int video_processor::CODEC_YUV = CV_FOURCC('I','Y','U','V');
const int video_processor::CODEC_MP4 = CV_FOURCC('m','p','4','v');
const int video_processor::CODEC_XVID = CV_FOURCC('X','V','I','D');
const int video_processor::CODEC_H264 = CV_FOURCC('H','2','6','4');
const int video_processor::DEFAULT_CODEC = video_processor::CODEC_MP4;

video_processor::video_processor() {
}

bool video_processor::get_info(const std::string& infile, video_info& info)
{
#ifndef WIN32
    key_t key = IPC_PRIVATE;
    int shmid = shmget(key, sizeof(video_info), IPC_CREAT | 0666);
    if (shmid == -1) {
      std::cerr << " !! Failed to create shared memory.\n";
      return false;
    }

    video_info* var = (video_info*) shmat(shmid, NULL, 0);
    if (! var) {
      std::cerr << " !! Failed to map shared memory.\n";
      return false;
    }
    var->init();

	pid_t pid = fork();
	if (pid == 0) {
		install_signal_actions();

		try {
			cv::VideoCapture cap(infile);
			if (! cap.isOpened()) {
				throw new std::exception();
			}
			info.codec = (int) cap.get(CV_CAP_PROP_FOURCC);
			info.width = (int) cap.get(CV_CAP_PROP_FRAME_WIDTH);
			info.height = (int) cap.get(CV_CAP_PROP_FRAME_HEIGHT);
			info.fps = (int) cap.get(CV_CAP_PROP_FPS);
			info.frame_count = cap.get(CV_CAP_PROP_FRAME_COUNT);
            info.copy(*var);
		} catch (std::exception& e) {
			std::cerr << "Failed to get video info: " << e.what() << std::endl;
            shmdt(var);
			exit(1);
		}
        shmdt(var);
		exit(0);
	} else {
	//	std::cerr << "VideoProcessor >> Waiting for pid " << pid << std::endl;
		int stat;
		pid_t child_pid = waitpid(pid, &stat, 0);
		if (child_pid == -1) {
			std::cerr << " VideoProcessor >> Child process " << pid << " failed." << std::endl;
			return false;
		} else if (stat != 0) {
			std::cerr << " VideoProcessor >> Child process " << pid << " finished with error " 
				<< (stat & 0xff00 >> 8) 
				<< "," << (stat & 0xff) << std::endl;
			return false;
		} else {
		//	std::cerr << " VideoProcessor >> Child process " << pid << " succeeded." << std::endl;
		}
	}
	return true;

#else
	try {
		cv::VideoCapture cap(infile);
		if (! cap.isOpened()) {
			throw new std::exception();
		}
		info.codec = (int) cap.get(CV_CAP_PROP_FOURCC);
		info.width = (int) cap.get(CV_CAP_PROP_FRAME_WIDTH);
		info.height = (int) cap.get(CV_CAP_PROP_FRAME_HEIGHT);
		info.fps = (int) cap.get(CV_CAP_PROP_FPS);
		info.frame_count = cap.get(CV_CAP_PROP_FRAME_COUNT);
	} catch (std::exception& e) {
		std::cerr << "Failed to get video info: " << e.what() << std::endl;
		return false;
	}
	return true;
#endif
}

bool video_processor::convert(const char* data, size_t length, 
	const std::string& outfile, video_info& info, int codec)
{
	if (! data || length == 0) {
		std::cerr << "Invalid argument to video_processor::convert" << std::endl;
		return false;
	}

	char* t = tmpnam(NULL);
	if (! t) {
		std::cerr << "Unable to get a temp file name!" << std::endl;
		return false;
	}
	//std::cerr << " >> Writing buffer to temp file: " << t << std::endl;
	std::fstream fs(t, std::ios_base::out | std::ios_base::binary);
	fs.write(data, length);

	bool ok = ! fs.bad();
	if (! ok) {
		std::cerr << " >> ERROR writing buffer to temp file: " << t << std::endl;
	}
	fs.close();

	std::string tmpfilename(t);

	//std::cerr << " >> start to convert " << tmpfilename << std::endl;
	ok &= convert(tmpfilename, outfile, info, codec);
	//std::cerr << " >> convertsion ended. remove " << tmpfilename << std::endl;

	if (0 != remove(tmpfilename.c_str())) {
		std::cerr << "Unable to remove tmp file " << tmpfilename << std::endl;
	}
	//std::cerr << " >> removed tmp file: " << tmpfilename << std::endl;

	return ok;
}

bool video_processor::convert(const std::string& infile, 
	const std::string& outfile, video_info& info, int codec) 
{
#ifndef WIN32
    key_t key = IPC_PRIVATE;
    int shmid = shmget(key, sizeof(video_info), IPC_CREAT | 0666);
    if (shmid == -1) {
      std::cerr << " !! Failed to create shared memory.\n";
      return false;
    }

    video_info* var = (video_info*) shmat(shmid, NULL, 0);
    if (! var) {
      std::cerr << " !! Failed to map shared memory.\n";
      return false;
    }
    var->init();

	pid_t pid = fork();
	if (pid == 0) {
		//std::cerr << " ** Start worker process "<<infile<<"->"<<outfile<<"\n";
		install_signal_actions();

		try {
			//std::cerr << " ** before opening input file\n";
			cv::VideoCapture cap(infile);
			if (! cap.isOpened()) {
				throw new std::exception();
			}
			//std::cerr << " ** input file opened\n";

			info.codec = (int) cap.get(CV_CAP_PROP_FOURCC);
			info.width = (int) cap.get(CV_CAP_PROP_FRAME_WIDTH);
			info.height = (int) cap.get(CV_CAP_PROP_FRAME_HEIGHT);
			info.fps = (int) cap.get(CV_CAP_PROP_FPS);
			info.frame_count = cap.get(CV_CAP_PROP_FRAME_COUNT);

			if (info.width == 0 || info.height == 0 || info.fps <= 1) {
				// unable to get video info from file. 
				// the file could not be processed by opencv.
				throw new std::exception();
			}

			if (codec == -1) {
				codec = info.codec;
			}
			//std::cerr << " ** Has video info: " << info.str() << std::endl;
            info.copy(*var);

			cv::Size size;
			size.width = info.width;
			size.height = info.height;

			//std::cerr << " ** writing to " << outfile << std::endl;

			cv::VideoWriter* writer =
				new cv::VideoWriter(outfile, codec, info.fps, size, true);
			if (! writer || ! writer->isOpened()) {
				throw new std::exception();
			}

			cv::Mat frame;
			while (cap.read(frame)) {
				(*writer) << frame;
			}
			//std::cerr << " ** Done writing to " << outfile << std::endl;
          
			delete writer;

		} catch (std::exception& e) {
			std::cerr << " ** Conversion failed: " << e.what() << std::endl;
            shmdt(var);
			exit(1);
		}

        shmdt(var);
		exit(0);

	} else {
		//std::cerr << " VideoProcessor >> Waiting for pid " << pid << std::endl;
		int stat;
		pid_t child_pid = waitpid(pid, &stat, 0);
		if (child_pid == -1) {
			std::cerr << " VideoProcessor >> Child process " << pid << " failed." << std::endl;
		} else if (stat != 0) {
			std::cerr << " VideoProcessor >> Child process " << pid << " finished with error " 
				<< (stat & 0xff00 >> 8) 
				<< "," << (stat & 0xff) << std::endl;
		} else {
		//	std::cerr << " VideoProcessor >> Child process " << pid << " succeeded." << std::endl;
            //std::cerr << " ** Has var parent: " << var->str() << "\n";
            var->copy(info);
		}
        shmdt(var);
	}

	//std::cerr << " >> Conversion done. Now test if it was successful.\n"; 

	std::fstream fs;
	fs.open(outfile.c_str(), std::ios_base::in | std::ios_base::binary);
	if (! fs.is_open()) {
		//std::cerr << "output file " << outfile << " couldn't be opened.\n";
		return false;
	}
	return true;

#else

	try {
		//std::cerr << " ** before opening input file\n";
		cv::VideoCapture cap(infile);
		if (! cap.isOpened()) {
			throw new std::exception();
		}
		//std::cerr << " ** input file opened\n";

		info.codec = (int) cap.get(CV_CAP_PROP_FOURCC);
		info.width = (int) cap.get(CV_CAP_PROP_FRAME_WIDTH);
		info.height = (int) cap.get(CV_CAP_PROP_FRAME_HEIGHT);
		info.fps = (int) cap.get(CV_CAP_PROP_FPS);
		info.frame_count = cap.get(CV_CAP_PROP_FRAME_COUNT);

		if (info.width == 0 || info.height == 0 || info.fps == 1) {
			// unable to get video info from file. 
			// the file could not be processed by opencv.
			throw new std::exception();
		}

		if (codec == -1) {
			codec = info.codec;
		}
		//std::cerr << " ** Has video info: width=" << info.str() << std::endl;

		cv::Size size;
		size.width = info.width;
		size.height = info.height;

		//std::cerr << " ** writing to " << outfile << std::endl;

		cv::VideoWriter* writer =
			new cv::VideoWriter(outfile, codec, info.fps, size, true);
		if (! writer || ! writer->isOpened()) {
			throw new std::exception();
		}

		cv::Mat frame;
		while (cap.read(frame)) {
			(*writer) << frame;
		}
		//std::cerr << " ** Done writing to " << outfile << std::endl;

		delete writer;

	} catch (std::exception& e) {
		std::cerr << " ** Conversion failed: " << e.what() << std::endl;
		return false;
	}

	return true;

#endif
}

bool video_processor::process_header(const sbuf_t& sbuf, size_t offset, 
	feature_recorder& recorder)
{
	/*std::cerr << " >> Processing header at " << sbuf.pos0 
		<< ", offset=" << offset << "\n";*/

	char* data = (char*) (sbuf.buf + offset);
	size_t l = sbuf.pagesize;
	size_t len = l-offset;
	const pos0_t &pos0 = sbuf.pos0;
	std::string fpath = pos0.path;

	// construct output file name
	std::stringstream s;
	s << recorder.name << "-p" << fpath << "-" << pos0.offset 
		<< "-" << offset << "-" << rand() << ".avi";
	std::string outfile = recorder.outdir + "/" + s.str();
	set_file_name(outfile);
	video_info info;
	bool ok = convert(data, len, outfile, info);
	if (ok) {
        //std::cerr << " ** Has outer video info: " << info.str() << std::endl;
		recorder.write(pos0+offset, info.str(), s.str());
		//recorder.write(pos0+offset, " ", s.str());
	} else {
		//  recorder.write(pos0+offset, "FAILED", "");
	}

	//std::cerr << " >> Done processing header at " << sbuf.pos0 
	//	<< ", offset=" << offset << "\n";

	return ok;
}

