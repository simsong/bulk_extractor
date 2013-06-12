bin_PROGRAMS   = bulk_extractor 
EXTRA_PROGRAMS = stand
CLEANFILES     = scan_accts.cpp scan_email.cpp scan_gps.cpp scan_base16.cpp *.d

AM_CPPFLAGS = -I${top_srcdir}/src/be13_api

include dfxml/src/Makefile.defs
include be13_api/Makefile.defs



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

RAR_SUPPORT = \
        rar/arccmt.cpp rar/archive.cpp rar/archive.hpp rar/arcread.cpp rar/array.hpp rar/cmddata.cpp \
        rar/cmddata.hpp rar/coder.cpp rar/coder.hpp rar/compress.hpp rar/crc.cpp rar/crc.hpp rar/encname.cpp \
        rar/encname.hpp rar/errhnd.cpp rar/errhnd.hpp rar/extinfo.cpp rar/extinfo.hpp rar/extract.cpp \
        rar/extract.hpp rar/file.cpp rar/filefn.cpp rar/filefn.hpp rar/file.hpp rar/filestr.cpp rar/filestr.hpp \
        rar/getbits.cpp rar/getbits.hpp rar/global.cpp rar/global.hpp rar/headers.hpp rar/loclang.hpp rar/match.cpp \
        rar/match.hpp rar/model.cpp rar/model.hpp rar/suballoc.cpp rar/suballoc.hpp rar/options.cpp rar/options.hpp \
        rar/os.hpp rar/pathfn.cpp rar/pathfn.hpp rar/rar.cpp rar/rardefs.hpp rar/rar.hpp rar/rarlang.hpp rar/raros.hpp \
        rar/rarpch.cpp rar/rartypes.hpp rar/rarvmtbl.cpp rar/rawread.cpp rar/rawread.hpp rar/rdwrfn.cpp rar/rdwrfn.hpp \
        rar/resource.cpp rar/resource.hpp rar/rs.cpp rar/rs.hpp rar/savepos.cpp rar/savepos.hpp rar/smallfn.cpp \
        rar/smallfn.hpp rar/strfn.cpp rar/strfn.hpp rar/strlist.cpp rar/strlist.hpp \
        rar/system.cpp rar/system.hpp rar/timefn.cpp rar/timefn.hpp rar/unicode.cpp \
        rar/unicode.hpp rar/unpack.cpp rar/unpack.hpp \
        rar/version.hpp

bulk_scanners = \
	scan_aes.cpp \
	scan_base64.cpp \
	scan_bulk.cpp\
	scan_ccns2.cpp scan_ccns2.h \
	scan_elf.cpp \
	scan_exif.cpp exif_reader.h exif_reader.cpp exif_entry.cpp\
	scan_exiv2.cpp \
	scan_extx.cpp \
	scan_find.cpp \
	scan_gzip.cpp \
	scan_hiberfile.cpp pyxpress.c pyxpress.h \
	scan_json.cpp \
	scan_kml.cpp \
	scan_lightgrep.cpp \
	scan_net.cpp \
	scan_pdf.cpp \
	scan_rar.cpp \
	scan_sectorid.cpp \
	scan_vcard.cpp \
	scan_windirs.cpp \
	scan_winpe.cpp \
	scan_winprefetch.cpp \
	scan_wordlist.cpp \
	scan_xor.cpp \
	scan_zip.cpp \
        sbuf_flex_scanner.h \
	$(flex_scanners) 

SCAN_LIFT = scan_lift.cpp \
	scan_lift/base.cpp \
	scan_lift/base.h \
	scan_lift/classifier_config.h \
	scan_lift/element.h \
	scan_lift/gflags.h \
	scan_lift/gflags_completions.h \
	scan_lift/irbase.cpp \
	scan_lift/lift_config.cpp \
	scan_lift/lift_config.h \
	scan_lift/lift_svm.cpp \
	scan_lift/lift_svm.h \
	scan_lift/lift_svm_trained.cpp \
	scan_lift/lift_utils.h \
	scan_lift/linear_binary_svm.cpp \
	scan_lift/linear_binary_svm.h \
	scan_lift/linear_ova_svm.cpp \
	scan_lift/linear_ova_svm.h \
	scan_lift/myheader.h \
	scan_lift/rawdefines.h \
	scan_lift/rawheaders.h \
	scan_lift/term_weighting.cpp \
	scan_lift/term_weighting.h \
	scan_lift/tools.cpp 

scan_lift.so: $(SCAN_LIFT)

bulk_extractor_SOURCES = \
	aftimer.h \
	base64_forensic.cpp \
	base64_forensic.h \
	bulk_extractor.cpp \
	bulk_extractor.h \
	dig.cpp \
	dig.h \
	histogram.cpp \
	histogram.h \
	image_process.cpp \
	image_process.h \
	support.cpp \
	threadpool.cpp \
	threadpool.h \
	word_and_context_list.cpp \
	word_and_context_list.h \
	$(bulk_scanners) $(TSK3INCS)  $(BE13_API) $(DFXML) # $(SCAN_LIFT) $(RAR_SUPPORT) 


stand_SOURCES = \
	dig.cpp \
	histogram.cpp \
	histogram.h \
	scan_bulk.cpp \
	stand.cpp \
	support.cpp \
	word_and_context_list.cpp \
	word_and_context_list.h \
	$(BE13_API)

SUFFIXES = .flex

src/scan_lift_trained.cpp:
	./bulk_extractor -E lift -s liftcmd=a2c -s liftif=scan_lift/model -s liftof=scan_lift/lift_svm_trained.cpp

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
	$(CXX) $(CPPFLAGS) $(AM_CPPFLAGS) $(CXXFLAGS) $(FLEX_CXXFLAGS) -I${top_srcdir} -I${srcdir} -I.. -I../../_build -c `basename $@ .o`.cpp 
