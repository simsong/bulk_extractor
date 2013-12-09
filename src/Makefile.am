bin_PROGRAMS   = bulk_extractor 
EXTRA_PROGRAMS = stand bulk_extractorlib beapi_demo
CLEANFILES     = scan_accts.cpp scan_email.cpp scan_gps.cpp scan_base16.cpp *.d

AM_CPPFLAGS = -I${top_srcdir}/src/be13_api

# http://stackoverflow.com/questions/11438613/conditional-subdir-objects
AUTOMAKE_OPTIONS = subdir-objects

# bring in defs from two included projects
include dfxml/src/Makefile.defs
include be13_api/Makefile.defs
include rar/Makefile.defs

# This was for building a test program
jpegdump: scan_exif.cpp be13_api/sbuf.o be13_api/unicode_escape.o 
	g++ -o jpegdump $(CPPFLAGS) $(CXXFLAGS) $(AM_CPPFLAGS) -I.. -DDUMPTEST scan_exif.cpp be13_api/sbuf.o be13_api/unicode_escape.o

TSK3INCS = \
	tsk3/auto/tsk_auto.h\
	tsk3/base/tsk_base.h\
	tsk3/base/tsk_os.h\
	tsk3/fs/tsk_fatfs.h\
	tsk3/fs/tsk_fs.h\
	tsk3/fs/tsk_ntfs.h\
	tsk3/hashdb/tsk_hashdb.h\
	tsk3/img/tsk_img.h\
	tsk3/libtsk.h\
	tsk3/tsk_incs.h\
	tsk3/vs/tsk_vs.h

flex_scanners = \
	scan_accts.flex \
	scan_base16.flex \
	scan_email.flex \
	scan_gps.flex 

bulk_scanners = \
	scan_accts_lg.cpp \
	scan_aes.cpp \
	scan_base16_lg.cpp \
	scan_base64.cpp \
	scan_bulk.cpp\
	scan_ccns2.cpp scan_ccns2.h \
	scan_elf.cpp \
	scan_email_lg.cpp \
	scan_exif.cpp exif_reader.h exif_reader.cpp exif_entry.cpp\
	scan_exiv2.cpp \
	scan_extx.cpp \
	scan_find.cpp \
	scan_gps_lg.cpp \
	scan_gzip.cpp \
	scan_hiberfile.cpp pyxpress.c pyxpress.h \
	scan_json.cpp \
	scan_kml.cpp \
	scan_lightgrep.cpp \
	scan_net.cpp \
	scan_pdf.cpp \
	scan_rar.cpp \
	scan_hashid.cpp \
	scan_vcard.cpp \
	scan_windirs.cpp \
	scan_winpe.cpp \
	scan_winprefetch.cpp \
	scan_wordlist.cpp \
	scan_xor.cpp \
	scan_zip.cpp \
	sbuf_flex_scanner.h \
	$(flex_scanners) 

if RAR_ENABLED
bulk_scanners += $(RAR_SUPPORT)
endif

bulk_extractor_nomain = \
	base64_forensic.cpp \
	base64_forensic.h \
	bulk_extractor.cpp \
	bulk_extractor.h \
	dig.cpp \
	dig.h \
	image_process.cpp \
	image_process.h \
	pattern_scanner.cpp \
	pattern_scanner.h \
	pattern_scanner_utils.cpp \
	pattern_scanner_utils.h \
	phase1.h \
	phase1.cpp \
	threadpool.cpp \
	threadpool.h \
	$(bulk_scanners) $(TSK3INCS)  $(BE13_API) $(DFXML_WRITER) 

bulk_extractor_SOURCES = $(bulk_extractor_nomain) main.cpp bulk_extractor_api.cpp

bulk_extractorlib_SOURCES = $(bulk_extractor_nomain) bulk_extractor_api.cpp

bulk_extractor.so: $(bulk_extractorlib_OBJECTS)
	$(CXX) -shared -fPIC -o $@ $(LDFLAGS) $(LIBS) $(bulk_extractorlib_OBJECTS)

stand_SOURCES = \
	dig.cpp \
	histogram.cpp \
	histogram.h \
	scan_bulk.cpp \
	stand.cpp \
	$(BE13_API) $(DFXML_WRITER) 

beapi_demo_SOURCES = beapi_demo.cpp
beapi_demo: bulk_extractor.so		# needs an up-to-date .so

SUFFIXES = .flex

digtest$(EXEEXT): dig.cpp
	$(CXX) -DSTANDALONE -o digtest$(EXEEXT) -g dig.cpp $(CPPFLAGS) $(CXXFLAGS) -I..

unitest$(EXEEXT): unicode_escape.cpp
	$(CXX) -DSTANDALONE -o unitest$(EXEEXT) -g unicode_escape.cpp  $(CPPFLAGS) $(CXXFLAGS) -I..

#
# Rule to compile .flex into a .o
# FreeBSD NOTE: flex does not support -R and -o does not work properly
# | sed "s/static int yy_get_next_buffer/static int XXyy_get_next_buffer/" 

FLEX_CXXFLAGS=-Wno-shadow -Wno-effc++ -Wno-redundant-decls -Wno-missing-noreturn
.flex.o:
	$(LEX) -R -t $<  > `basename $@ .o`.cpp 
	$(CXX) $(CPPFLAGS) $(AM_CPPFLAGS) $(CXXFLAGS) $(FLEX_CXXFLAGS) \
		-I${top_srcdir} -I${srcdir} -I.. -I../../_build -c `basename $@ .o`.cpp 
