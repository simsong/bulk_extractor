bin_PROGRAMS   = bulk_extractor 
EXTRA_PROGRAMS = stand
CLEANFILES     = scan_accts.cpp scan_email.cpp scan_gps.cpp scan_base16.cpp *.d

AM_CPPFLAGS = -I${top_srcdir}/src/be13_api

# scan_lift/type_identifier.tex

#extradir = .

BE13_API = \
        be13_api/bulk_extractor_i.h \
        be13_api/cppmutex.h \
        be13_api/feature_recorder.cpp \
        be13_api/feature_recorder.h \
        be13_api/feature_recorder_set.cpp \
        be13_api/feature_recorder_set.h \
        be13_api/md5.c \
        be13_api/md5.h \
        be13_api/pcap_fake.h \
        be13_api/pcap_fake.cpp \
        be13_api/plugin.cpp\
	be13_api/beregex.cpp \
	be13_api/beregex.h \
        be13_api/sbuf.cpp \
        be13_api/sbuf.h \
        be13_api/sbuf_private.h \
        be13_api/sbuf_stream.h \
        be13_api/sbuf_stream.cpp \
        be13_api/utf8.h \
        be13_api/utf8/checked.h \
        be13_api/utf8/core.h \
        be13_api/utf8/unchecked.h \
        be13_api/xml.cpp \
        be13_api/xml.h \
	be13_api/unicode_escape.cpp\
	be13_api/unicode_escape.h \
	be13_api/utils.h \
	be13_api/utils.cpp

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
        rar/archive.hpp \
        rar/array.hpp \
        rar/cmddata.hpp \
        rar/coder.hpp \
        rar/compress.hpp \
        rar/consio.hpp \
        rar/crc.hpp \
        rar/crypt.hpp \
        rar/dll.hpp \
        rar/encname.hpp \
        rar/errhnd.hpp \
        rar/extinfo.hpp \
        rar/extract.hpp \
        rar/filcreat.hpp \
        rar/filefn.hpp \
        rar/file.hpp \
        rar/filestr.hpp \
        rar/find.hpp \
        rar/getbits.hpp \
        rar/global.hpp \
        rar/headers.hpp \
        rar/isnt.hpp \
        rar/list.hpp \
        rar/loclang.hpp \
        rar/log.hpp \
        rar/match.hpp \
        rar/model.hpp \
        rar/options.hpp \
        rar/os.hpp \
        rar/pathfn.hpp \
        rar/rardefs.hpp \
        rar/rar.hpp \
        rar/rarlang.hpp \
        rar/raros.hpp \
        rar/rartypes.hpp \
        rar/rarvm.hpp \
        rar/rawread.hpp \
        rar/rdwrfn.hpp \
        rar/recvol.hpp \
        rar/resource.hpp \
        rar/rijndael.hpp \
        rar/rs.hpp \
        rar/savepos.hpp \
        rar/scantree.hpp \
        rar/sha1.hpp \
        rar/smallfn.hpp \
        rar/strfn.hpp \
        rar/strlist.hpp \
        rar/suballoc.hpp \
        rar/system.hpp \
        rar/timefn.hpp \
        rar/ulinks.hpp \
        rar/unicode.hpp \
        rar/unpack.hpp \
        rar/version.hpp \
        rar/volume.hpp \
        rar/arccmt.cpp \
        rar/archive.cpp \
        rar/arcread.cpp \
        rar/beosea.cpp \
        rar/cmddata.cpp \
        rar/coder.cpp \
        rar/consio.cpp \
        rar/crc.cpp \
        rar/crypt.cpp \
        rar/dll.cpp \
        rar/encname.cpp \
        rar/errhnd.cpp \
        rar/extinfo.cpp \
        rar/extract.cpp \
        rar/filcreat.cpp \
        rar/file.cpp \
        rar/filefn.cpp \
        rar/filestr.cpp \
        rar/find.cpp \
        rar/getbits.cpp \
        rar/global.cpp \
        rar/isnt.cpp \
        rar/list.cpp \
        rar/log.cpp \
        rar/match.cpp \
        rar/model.cpp \
        rar/options.cpp \
        rar/os2ea.cpp \
        rar/pathfn.cpp \
        rar/rar.cpp \
        rar/rarpch.cpp \
        rar/rarvm.cpp \
        rar/rarvmtbl.cpp \
        rar/rawread.cpp \
        rar/rdwrfn.cpp \
        rar/recvol.cpp \
        rar/resource.cpp \
        rar/rijndael.cpp \
        rar/rs.cpp \
        rar/savepos.cpp \
        rar/scantree.cpp \
        rar/sha1.cpp \
        rar/smallfn.cpp \
        rar/strfn.cpp \
        rar/strlist.cpp \
        rar/suballoc.cpp \
        rar/system.cpp \
        rar/timefn.cpp \
        rar/ulinks.cpp \
        rar/unicode.cpp \
        rar/unios2.cpp \
        rar/unpack15.cpp \
        rar/unpack20.cpp \
        rar/unpack.cpp \
        rar/uowners.cpp \
        rar/volume.cpp \
        rar/win32acl.cpp \
        rar/win32stm.cpp

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
	scan_vcard.cpp \
	scan_windirs.cpp \
	scan_winpe.cpp \
	scan_winprefetch.cpp \
	scan_wordlist.cpp \
	scan_xor.cpp \
	scan_zip.cpp \
        sbuf_flex_scanner.h \
        $(RAR_SUPPORT) \
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
	$(bulk_scanners) $(TSK3INCS)  $(BE13_API)

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
