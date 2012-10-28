#
# make bulk_extractor plugins
# stand

BE13_API = be13_api/bulk_extractor_i.h \
	be13_api/cppmutex.h \
	be13_api/feature_recorder.h \
	be13_api/feature_recorder_set.h \
	be13_api/md5.h \
	be13_api/sbuf.h \
	be13_api/sbuf_flex_scanner.h \
	be13_api/sbuf_private.h \
	be13_api/sbuf_stream.h \
	be13_api/utf8/checked.h \
	be13_api/utf8/core.h \
	be13_api/utf8/unchecked.h 

AM_CPPFLAGS =  -Ibe13_api 

bin_PROGRAMS = demo 

demo_SOURCES = demo.cpp $(BE13_API)

scan_demo.so: scan_demo.cpp
scan_flexdemo.so: scan_flexdemo.flex

#
# Rule to compile .flex into a .o
# 

SUFFIXES = .so .flex
.cpp.so:
	$(CXXCOMPILE) -shared -fPIC -o $@ $(CXXFLAGS) $<

FLEX_CXXFLAGS=-Wno-shadow -Wno-effc++ -Wno-redundant-decls -Wno-missing-noreturn
.flex.so:
	flex -o `basename $@ .so`.cpp $<  
	$(CXXCOMPILE) $(FLEX_CXXFLAGS) -shared -fPIC $(CXXFLAGS) -o $@ `basename $@ .so`.cpp 2>&1|egrep -v 'redundant redeclaration|previous declaration'
