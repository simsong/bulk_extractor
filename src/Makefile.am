bin_PROGRAMS   = bulk_extractor 
EXTRA_PROGRAMS = stand bulk_extractorlib beapi_demo bulk_extractor_noscanners
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

# These scanners are based on GNUflex
flex_scanners = \
	sbuf_flex_scanner.h \
	scan_accts.flex \
	scan_base16.flex \
	scan_email.flex \
	scan_gps.flex 


# These scanners are based on Lightbox Technology's lightgrep
lightgrep_scanners = \
	pattern_scanner.cpp pattern_scanner.h \
	pattern_scanner_utils.cpp pattern_scanner_utils.h \
	scan_lightgrep.cpp \
	scan_accts_lg.cpp \
	scan_base16_lg.cpp \
	scan_email_lg.cpp \
	scan_gps_lg.cpp 

bulk_scanners = \
	scan_aes.cpp \
	scan_base64.cpp \
	scan_ccns2.cpp scan_ccns2.h \
	scan_elf.cpp \
	scan_evtx.cpp \
	scan_exif.cpp exif_reader.h exif_reader.cpp exif_entry.cpp\
	scan_exiv2.cpp \
	scan_facebook.cpp \
	scan_find.cpp \
	scan_gzip.cpp \
	scan_hashdb.cpp \
	scan_hiberfile.cpp pyxpress.c pyxpress.h \
	scan_httplogs.cpp \
	scan_json.cpp \
	scan_kml.cpp \
	scan_msxml.cpp \
	scan_net.cpp \
	scan_ntfsindx.cpp \
	scan_ntfslogfile.cpp \
	scan_ntfsmft.cpp \
	scan_ntfsusn.cpp \
	scan_outlook.cpp \
	scan_pdf.cpp \
	scan_rar.cpp \
	scan_sceadan.cpp\
	scan_sqlite.cpp \
	scan_utmp.cpp \
	scan_vcard.cpp \
	scan_windirs.cpp \
	scan_winlnk.cpp \
	scan_winpe.cpp \
	scan_winprefetch.cpp \
	scan_wordlist.cpp \
	scan_xor.cpp \
	scan_zip.cpp 

if FLEXSCANNERS_ENABLED
  bulk_scanners += $(flex_scanners) 
endif

if LIGHTGREP_ENABLED
  bulk_scanners += $(lightgrep_scanners)
endif


if RAR_ENABLED
  bulk_scanners += $(RAR_SUPPORT)
endif

bulk_extractor_nomain = \
	bulk_extractor_api.cpp bulk_extractor_api.h \
	base64_forensic.cpp \
	base64_forensic.h \
	bulk_extractor.h \
	dig.cpp \
	dig.h \
	findopts.h \
	findopts.cpp \
	image_process.cpp \
	image_process.h \
	phase1.h \
	phase1.cpp \
	threadpool.cpp \
	threadpool.h \
	$(TSK3INCS)  $(BE13_API) $(DFXML_WRITER) 

bulk_extractor_SOURCES =    $(bulk_extractor_nomain) $(bulk_scanners) bulk_extractor_scanners.cpp main.cpp 

bulk_extractorlib_SOURCES = $(bulk_extractor_nomain) $(bulk_scanners) bulk_extractor_scanners.cpp 

bulk_extractor_noscanners_SOURCES = $(bulk_extractor_nomain) bulk_extractor_noscanners.cpp main.cpp 

stand_SOURCES = dig.cpp scan_sceadan.cpp stand.cpp $(TSK3INCS) $(BE13_API) $(DFXML_WRITER) 

lib: libbulkextractor.so

libbulkextractor.so: $(bulk_extractorlib_OBJECTS)
	$(CXX) -shared -fPIC -o $@ $(LDFLAGS) $(bulk_extractorlib_OBJECTS) $(LIBS)


beapi_demo_SOURCES = beapi_demo.cpp
beapi_demo$(EXEEXT): libbulkextractor.so		# needs an up-to-date .so

rd: beapi_demo libbulkextractor.so
	echo run the demo
	./beapi_demo /corp/nps/drives/nps-2010-emails/nps-2010-emails.raw


digtest$(EXEEXT): dig.cpp
	$(CXX) -DSTANDALONE -o digtest$(EXEEXT) -g dig.cpp $(CPPFLAGS) $(CXXFLAGS) -I..

unitest$(EXEEXT): unicode_escape.cpp
	$(CXX) -DSTANDALONE -o unitest$(EXEEXT) -g unicode_escape.cpp  $(CPPFLAGS) $(CXXFLAGS) -I..

#
# Rule to compile .flex into a .o
# FreeBSD NOTE: flex does not support -R and -o does not work properly
# | sed "s/static int yy_get_next_buffer/static int XXyy_get_next_buffer/" 
#
# 2019-11-09 - updated to remove the word 'register' which is no longer needed

SUFFIXES = .flex
FLEX_CXXFLAGS=-Wno-shadow -Wno-effc++ -Wno-redundant-decls -Wno-missing-noreturn
.flex.o:
	$(LEX) -R -t $<  | sed "s/register //" > `basename $@ .o`.cpp 
	$(CXX) $(CPPFLAGS) $(AM_CPPFLAGS) $(CXXFLAGS) $(FLEX_CXXFLAGS) \
		-I${top_srcdir} -I${srcdir} -I.. -I../../_build -c `basename $@ .o`.cpp 

