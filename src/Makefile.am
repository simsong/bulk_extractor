bin_PROGRAMS   = bulk_extractor be_tests

CLEANFILES     = scan_accts.cpp scan_base16.cpp scan_email.cpp scan_gps.cpp \
	be13_api/config.h be13_api/dfxml/src/config.h config.h *.d *~

TESTS = be_tests
check_PROGRAMS = be_tests

# So that relative imports from be13_api work
AM_CPPFLAGS = -I${top_srcdir}/src/be13_api

AUTOMAKE_OPTIONS = subdir-objects

# bring in defs from two included projects
# note: don't be clever and use the $variable on the Include line. it doesn't work


BE13_API_DIR= be13_api
include be13_api/Makefile.defs


#include rar/Makefile.defs

#jpeg_dump_SOURCES = 	scan_exif.cpp scan_exif.h exif_reader.cpp exif_reader.h exif_entry.cpp\
#	be13_api/sbuf.h be13_api/sbuf.cpp be13_api/unicode_escape.o jpeg_dump.cpp

# TSK3INCS = \
# 	tsk3/auto/tsk_auto.h\
# 	tsk3/base/tsk_base.h\
# 	tsk3/base/tsk_os.h\
# 	tsk3/fs/tsk_fatfs.h\
# 	tsk3/fs/tsk_fs.h\
# 	tsk3/fs/tsk_ntfs.h\
# 	tsk3/hashdb/tsk_hashdb.h\
# 	tsk3/img/tsk_img.h\
# 	tsk3/libtsk.h\
# 	tsk3/tsk_incs.h\
# 	tsk3/vs/tsk_vs.h
#


# These scanners are based on GNUflex
flex_scanners = \
	sbuf_flex_scanner.h \
	scan_base16.flex \
	scan_accts.flex \
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

# scanners_builtin are the scanners that are compiled into the binary

scanners_builtin = \
	bulk_extractor_scanners.cpp \
	scan_aes.cpp \
	scan_base64.cpp \
	scan_ccns2.cpp scan_ccns2.h \
	scan_elf.cpp \
	scan_exif.cpp scan_exif.h exif_reader.cpp exif_reader.h exif_entry.cpp jpeg_validator.h \
	scan_facebook.cpp \
	scan_gzip.cpp \
	scan_httplogs.cpp \
	scan_json.cpp \
	scan_pdf.cpp scan_pdf.h \
	scan_vcard.cpp \
	scan_xor.cpp \
	scan_zip.cpp

not_used_yet = \
	scan_evtx.cpp \
	scan_exiv2.cpp \
	scan_find.cpp \
	scan_hiberfile.cpp pyxpress.c pyxpress.h \
	scan_kml.cpp \
	scan_msxml.cpp \
	scan_net.cpp \
	scan_ntfsindx.cpp \
	scan_ntfslogfile.cpp \
	scan_ntfsmft.cpp \
	scan_ntfsusn.cpp \
	scan_outlook.cpp scan_outlook.h \
	scan_rar.cpp \
	scan_sqlite.cpp \
	scan_utmp.cpp \
	scan_windirs.cpp \
	scan_winlnk.cpp \
	scan_winpe.cpp \
	scan_winprefetch.cpp \
	scan_wordlist.cpp


if FLEXSCANNERS_ENABLED
  scanners_builtin += $(flex_scanners)
endif

if LIGHTGREP_ENABLED
  scanners_builtin += $(lightgrep_scanners)
endif


if RAR_ENABLED
  scanners_builtin += $(RAR_SUPPORT)
endif

bulk_extractor_nomain = \
        $(TSK3INCS)  \
	$(BE13_API_SRC) \
	bulk_extractor_scanners.h \
	bulk_extractor_api.cpp \
	bulk_extractor_api.h \
	base64_forensic.cpp \
	base64_forensic.h \
	bulk_extractor.h \
	findopts.h \
	image_process.cpp \
	image_process.h \
	phase1.h \
	phase1.cpp \
	sbuf_decompress.cpp \
	sbuf_decompress.h \
	threadpool.hpp

bulk_extractor_SOURCES = $(bulk_extractor_nomain) $(scanners_builtin) main.cpp
be_tests_SOURCES = $(bulk_extractor_nomain) $(scanners_builtin) be13_api/catch.hpp be_tests.cpp

# bulk_extractorlib_SOURCES = $(bulk_extractor_nomain) $(scanners_builtin) bulk_extractor_scanners.cpp
# bulk_extractor_noscanners_SOURCES = $(bulk_extractor_nomain) bulk_extractor_noscanners.cpp main.cpp

# stand is the standalone program that just runs on a single file.
# stand_SOURCES = stand.cpp $(TSK3INCS) $(BE13_API_SRC)  scan_json.cpp

#lib: libbulkextractor.so

#libbulkextractor.so: $(bulk_extractorlib_OBJECTS)
#	$(CXX) -shared -fPIC -o $@ $(LDFLAGS) $(bulk_extractorlib_OBJECTS) $(LIBS)

#beapi_demo_SOURCES = beapi_demo.cpp
#beapi_demo$(EXEEXT): libbulkextractor.so		# needs an up-to-date .so
#
#rd: beapi_demo libbulkextractor.so
#	echo run the demo
#	./beapi_demo /corp/nps/drives/nps-2010-emails/nps-2010-emails.raw

#unitest$(EXEEXT): unicode_escape.cpp
#	$(CXX) -DSTANDALONE -o unitest$(EXEEXT) -g unicode_escape.cpp  $(CPPFLAGS) $(CXXFLAGS) -I..

#
# Rule to compile .flex into a .o
# FreeBSD NOTE: flex does not support -R and -o does not work properly
# | sed "s/static int yy_get_next_buffer/static int XXyy_get_next_buffer/"
#

SUFFIXES = .flex
FLEX_CXXFLAGS=-Wno-shadow -Wno-effc++ -Wno-redundant-decls -Wno-missing-noreturn
.flex.o:
	$(LEX) -R -t $<  | sed "s/register //" > `basename $@ .o`.cpp
	$(CXX) $(CPPFLAGS) $(AM_CPPFLAGS) $(CXXFLAGS) $(FLEX_CXXFLAGS) \
		-I${top_srcdir} -I${srcdir} -I.. -I../../_build -c `basename $@ .o`.cpp
