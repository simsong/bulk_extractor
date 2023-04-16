bin_PROGRAMS   = bulk_extractor test_be

CLEANFILES     = scan_accts.cpp scan_base16.cpp scan_email.cpp scan_gps.cpp \
	be20_api/config.h be20_api/dfxml/src/config.h config.h *.d *~

TESTS = test_be
check_PROGRAMS = test_be

# bring in defs from two included projects
# note: don't be clever and use the $variable on the Include line. it doesn't work

BE20_API_DIR= be20_api
include be20_api/Makefile.defs
include rar/Makefile.defs

# So that relative imports from be20_api work
AM_CPPFLAGS = -I$(top_srcdir)/src/be20_api -I$(top_srcdir)/src/be20_api/utfcpp/source
AUTOMAKE_OPTIONS = subdir-objects
EXTRA_DIST = .gitignore $(BE20_API_EXTRA_DIST)

TSK3INCS = \
	tsk3_fatdirs.h\
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
	scan_base16.flex \
	scan_accts.flex \
	scan_email.flex scan_email.h \
	scan_gps.flex

# This is gross. Sorry!
scan_base16.o: sbuf_flex_scanner.h
scan_accts.o: sbuf_flex_scanner.h
scan_email.o: sbuf_flex_scanner.h
scan_gps.o: sbuf_flex_scanner.h

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
	scan_aes.cpp scan_aes.h \
	scan_base64.cpp scan_base64.h \
	scan_ccns2.cpp scan_ccns2.h \
	scan_elf.cpp \
	scan_evtx.cpp \
	scan_exif.cpp scan_exif.h exif_reader.cpp exif_reader.h exif_entry.h exif_entry.cpp jpeg_validator.h \
	scan_exiv2.cpp \
	scan_facebook.cpp \
	scan_find.cpp \
	scan_gzip.cpp \
	scan_hiberfile.cpp pyxpress.c pyxpress.h \
	scan_httplogs.cpp \
	scan_json.cpp \
	scan_kml.cpp \
	scan_msxml.cpp scan_msxml.h \
	scan_net.cpp scan_net.h \
	scan_ntfsindx.cpp \
	scan_ntfslogfile.cpp \
	scan_ntfsmft.cpp \
	scan_ntfsusn.cpp \
	scan_outlook.cpp scan_outlook.h \
	scan_pdf.cpp scan_pdf.h \
	scan_rar.cpp \
	scan_sqlite.cpp \
	scan_utmp.cpp \
	scan_vcard.cpp scan_vcard.h \
	scan_windirs.cpp \
	scan_winlnk.cpp \
	scan_winpe.cpp \
	scan_winprefetch.cpp \
	scan_wordlist.cpp scan_wordlist.h \
	scan_xor.cpp \
	scan_zip.cpp \
	pcap_writer.cpp \
	pcap_writer.h

if FLEXSCANNERS_ENABLED
  scanners_builtin += $(flex_scanners)
endif

if LIGHTGREP_ENABLED
  scanners_builtin += $(lightgrep_scanners)
endif

if RAR_ENABLED
  scanners_builtin += $(RAR_SUPPORT)
endif

bulk_extractor_parts = \
        $(TSK3INCS)  \
	$(BE20_API_SRC) \
	bulk_extractor_restarter.h \
	bulk_extractor_scanners.h \
	base64_forensic.cpp \
	base64_forensic.h \
	bulk_extractor.cpp \
	bulk_extractor.h \
	cxxopts.hpp \
	image_process.cpp \
	image_process.h \
	notify_thread.cpp \
	notify_thread.h \
	phase1.h \
	phase1.cpp \
	sbuf_decompress.cpp \
	sbuf_decompress.h


bulk_extractor_SOURCES = $(bulk_extractor_parts) $(scanners_builtin) main.cpp
test_be_SOURCES = $(bulk_extractor_parts) $(scanners_builtin) be20_api/catch.hpp test_be.h test_be1.cpp test_be2.cpp test_be3.cpp


runs.txt: test_be tests/run_each.sh
	bash tests/run_each.sh > runs.txt 2>&1

#lib: libbulkextractor.so

#libbulkextractor.so: $(bulk_extractorlib_OBJECTS)
#	$(CXX) -shared -fPIC -o $@ $(LDFLAGS) $(bulk_extractorlib_OBJECTS) $(LIBS)

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
	@echo "  FLEX-CXX" $@
	@$(LEX) -R -t $<  | sed "s/register //" > `basename $@ .o`.cpp
	@$(CXX) $(CPPFLAGS) $(AM_CPPFLAGS) $(CXXFLAGS) $(FLEX_CXXFLAGS) \
		-I${top_srcdir} -I${srcdir} -I.. -I../../_build -c `basename $@ .o`.cpp
