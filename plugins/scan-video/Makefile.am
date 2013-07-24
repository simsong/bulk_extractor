# See http://sources.redhat.com/autobook/autobook/autobook_88.html

# http://stackoverflow.com/questions/917948/automake-and-files-with-the-same-name
AUTOMAKE_OPTIONS = subdir-objects

AM_CFLAGS=-I../src -I../
AM_CXXFLAGS=-I../src -I../

AM_CXXFLAGS+=-fPIC -D__STDC_CONSTANT_MACROS

EXTRA_DIST = scan_bulk.cpp demo.cpp hello.cpp scan_rar.cpp scan_hello.cpp scan_avi.cpp scan_mp4.cpp scan_3gp.cpp scan_3g2.cpp scan_asf.cpp scan_mov.cpp video_processor.cpp isomf.cpp isomf_factory.cpp fix_mp4.cpp h264_model.cpp bitstream_reader.cpp bitstream_writer.cpp extract_keyframes.cpp process_histogram.cpp keyframe_utils.cpp bytestream_reader.cpp
# scan_lzma.cpp
PLUGINS = libvideo_util.so scan_bulk.so scan_rar.so scan_hello.so scan_avi.so scan_mp4.so scan_3gp.so scan_3g2.so scan_asf.so scan_mov.so 

# I could use libutil:
# http://stackoverflow.com/questions/5671681/how-do-you-make-a-so-library-with-autoconf-rather-that-a-la-library/5674708#5674708
# but what we have below seems to work
# pkglib_LTLIBRARIES = scan_bulk.la
# scan_bulk_la_LDFLAGS = -module -avoid-version -shar
#lib_LTLIBRARIES = libscan_bulk.la
#libscan_bulk_la_SOURCES = scan_bulk.cpp

pkglib_LTLIBRARIES = 

#pkglib_LTLIBRARIES += libvideo_util.la
#libvideo_util_la_LDFLAGS = -avoid-version -shar
#libvideo_util_la_SOURCES = video_processor.cpp isomf.cpp isomf_factory.cpp fix_mp4.cpp extract_keyframes.cpp process_histogram.cpp

pkglib_LTLIBRARIES += scan_hello.la
scan_hello_la_LDFLAGS = -module -avoid-version -shar
scan_hello_la_SOURCES = scan_hello.cpp

pkglib_LTLIBRARIES += scan_avi.la
scan_avi_la_LDFLAGS = -module -avoid-version -shar
scan_avi_la_SOURCES = scan_avi.cpp

pkglib_LTLIBRARIES += scan_mp4.la
scan_mp4_la_LDFLAGS = -module -avoid-version -shar
scan_mp4_la_SOURCES = scan_mp4.cpp

pkglib_LTLIBRARIES += scan_3gp.la
scan_3gp_la_LDFLAGS = -module -avoid-version -shar
scan_3gp_la_SOURCES = scan_3gp.cpp

pkglib_LTLIBRARIES += scan_3g2.la
scan_3g2_la_LDFLAGS = -module -avoid-version -shar
scan_3g2_la_SOURCES = scan_3g2.cpp

pkglib_LTLIBRARIES += scan_asf.la
scan_asf_la_LDFLAGS = -module -avoid-version -shar
scan_asf_la_SOURCES = scan_asf.cpp

pkglib_LTLIBRARIES += scan_mov.la
scan_mov_la_LDFLAGS = -module -avoid-version -shar
scan_mov_la_SOURCES = scan_mov.cpp

all:
	echo type \"make plugins\" to make the plugins
	echo type \"make demo\" to make the shared library demo

plugins: $(PLUGINS)

# LZMA Support
# This is largely taken from AFFLIB's LZMA support.

#INCLUDES =  \
#	-I@top_srcdir@/lzma443/C  \
#	-I@top_srcdir@/lzma443/C/7zip/Compress/LZMA_Alone  

#SLZMA =  \
#	scan_lzma.cpp \
#	@top_srcdir@/plugins/lzma443/C/7zip/Compress/LZMA_Alone/LzmaBench.cpp \
#	@top_srcdir@/plugins/lzma443/C/7zip/Compress/LZMA_Alone/LzmaRam.cpp \
#	@top_srcdir@/plugins/lzma443/C/7zip/Compress/LZMA_Alone/LzmaRamDecode.c \
#	@top_srcdir@/plugins/lzma443/C/7zip/Compress/LZMA_C/LzmaDecode.c \
#	@top_srcdir@/plugins/lzma443/C/7zip/Compress/Branch/BranchX86.c \
#	@top_srcdir@/plugins/lzma443/C/7zip/Compress/LZMA/LZMADecoder.cpp \
#	@top_srcdir@/plugins/lzma443/C/7zip/Compress/LZMA/LZMAEncoder.cpp \
#	@top_srcdir@/plugins/lzma443/C/7zip/Compress/LZ/LZInWindow.cpp \
#	@top_srcdir@/plugins/lzma443/C/7zip/Compress/LZ/LZOutWindow.cpp \
#	@top_srcdir@/plugins/lzma443/C/7zip/Compress/RangeCoder/RangeCoderBit.cpp \
#	@top_srcdir@/plugins/lzma443/C/7zip/Common/InBuffer.cpp \
#	@top_srcdir@/plugins/lzma443/C/7zip/Common/OutBuffer.cpp \
#	@top_srcdir@/plugins/lzma443/C/7zip/Common/StreamUtils.cpp \
#	@top_srcdir@/plugins/lzma443/C/Common/Alloc.cpp \
#	@top_srcdir@/plugins/lzma443/C/Common/CRC.cpp \
#	@top_srcdir@/plugins/lzma443/C/Common/Vector.cpp 


#scan_lzma.so: libscan_lzma.la
#	cp libscan_lzma.la scan_lzma.so

#scan_bulk.so: scan_bulk.cpp
scan_hello.so: scan_hello.cpp
scan_avi.so: scan_avi.cpp 
scan_mp4.so: scan_mp4.cpp
scan_3gp.so: scan_3gp.cpp
scan_3g2.so: scan_3g2.cpp
scan_asf.so: scan_asf.cpp
scan_mov.so: scan_mov.cpp

OPENCV_LIBS=-lopencv_core -lopencv_highgui -lopencv_ml -lopencv_video -lopencv_imgproc
FFMPEG_LIBS=-lavcodec -lavdevice -lavfilter -lavformat -lavutil -lswscale

video_processor.o: video_processor.cpp video_processor.h
	g++ -fPIC $(CXXFLAGS) $(CPPFLAGS) -I${top_srcdir} -I${srcdir} -I.. -c video_processor.cpp -o video_processor.o 

isomf.o: isomf.cpp isomf.h type_io.h
	g++ -fPIC $(CXXFLAGS) $(CPPFLAGS) -I${top_srcdir} -I${srcdir} -I.. -c isomf.cpp -o isomf.o 

h264_model.o: h264_model.cpp h264_model.h isomf.h type_io.h
	g++ -fPIC $(CXXFLAGS) $(CPPFLAGS) -I${top_srcdir} -I${srcdir} -I.. -c h264_model.cpp -o h264_model.o 

bitstream_reader.o: bitstream_reader.cpp bitstream_reader.h 
	g++ -fPIC $(CXXFLAGS) $(CPPFLAGS) -I${top_srcdir} -I${srcdir} -I.. -c bitstream_reader.cpp -o bitstream_reader.o 

bitstream_writer.o: bitstream_writer.cpp bitstream_writer.h 
	g++ -fPIC $(CXXFLAGS) $(CPPFLAGS) -I${top_srcdir} -I${srcdir} -I.. -c bitstream_writer.cpp -o bitstream_writer.o 

bytestream_reader.o: bytestream_reader.cpp bytestream_reader.h 
	g++ -fPIC $(CXXFLAGS) $(CPPFLAGS) -I${top_srcdir} -I${srcdir} -I.. -c bytestream_reader.cpp -o bytestream_reader.o 

isomf_factory.o: isomf_factory.cpp isomf_factory.h 
	g++ -fPIC $(CXXFLAGS) $(CPPFLAGS) -I${top_srcdir} -I${srcdir} -I.. -c isomf_factory.cpp -o isomf_factory.o 

fix_mp4.o: fix_mp4.cpp fix_mp4.h 
	g++ -fPIC -D__STDC_CONSTANT_MACROS  $(CXXFLAGS) $(CPPFLAGS) -I${top_srcdir} -I${srcdir} -I.. -c fix_mp4.cpp -o fix_mp4.o 

keyframe_utils.o: keyframe_utils.cpp keyframe_utils.h
	g++ -fPIC -D__STDC_CONSTANT_MACROS $(CXXFLAGS) $(CPPFLAGS) -I${top_srcdir} -I${srcdir} -I.. -c keyframe_utils.cpp -o keyframe_utils.o 

extract_keyframes.o: extract_keyframes.cpp extract_keyframes.h
	g++ -fPIC -D__STDC_CONSTANT_MACROS $(CXXFLAGS) $(CPPFLAGS) -I${top_srcdir} -I${srcdir} -I.. -c extract_keyframes.cpp -o extract_keyframes.o 

histogram_process.o: histogram_process.cpp histogram_process.h
	g++ -fPIC -D__STDC_CONSTANT_MACROS $(CXXFLAGS) $(CPPFLAGS) -I${top_srcdir} -I${srcdir} -I.. -c histogram_process.cpp -o histogram_process.o 

VIDEO_UTIL_OBJS=video_processor.o isomf.o isomf_factory.o fix_mp4.o h264_model.o bitstream_reader.o bitstream_writer.o extract_keyframes.o histogram_process.o keyframe_utils.o bytestream_reader.o

libvideo_util.so: $(VIDEO_UTIL_OBJS)
	g++ -shared -fPIC $(CXXFLAGS) $(CPPFLAGS) $(LDFLAGS) -o libvideo_util.so $(VIDEO_UTIL_OBJS) $(OPENCV_LIBS) $(FFMPEG_LIBS)

demo: demo.cpp hello.so
	g++ -o demo demo.cpp -ldl

# lib_LTLIBRARIES = libscan_lzma.la
# libscan_lzma_la_SOURCES = $(SLZMA)

CLEANFILES = scan_rar.so scan_bulk.so.dSYM scan_rar.so.dSYM scan_hello.so.dSYM scan_avi.so.dSYM scan_3gp.so.dSYM scan_mp4.so.dSYM *.d

SUFFIXES = .so
.cpp.so: libvideo_util.so
	g++ -shared -fPIC  -D__STDC_CONSTANT_MACROS -o $@ $(CXXFLAGS) $(CPPFLAGS) $(LDFLAGS) -I${top_srcdir} -I${srcdir} -I.. -L. $< $(OPENCV_LIBS) $(FFMPEG_LIBS) -lvideo_util

