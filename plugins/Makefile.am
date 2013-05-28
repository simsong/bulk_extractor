#
# make bulk_extractor plugins
# stand

AM_CPPFLAGS =  -I../src/be13_api  -I../src/

bin_PROGRAMS = plugin_test scan_demo.so scan_flexdemo.so

plugin_test_SOURCES = plugin_test.cpp

scan_demo.so: scan_demo.cpp
scan_flexdemo.so: scan_flexdemo.flex

scan_demo.dll: scan_demo.cpp
scan_flexdemo.dll: scan_flexdemo.flex

#
# Rule to compile .flex into a .o
# 

SUFFIXES = .so .flex
.cpp.so:
	$(CXXCOMPILE) -shared -fPIC -o $@ $(CXXFLAGS) $<

FLEX_CXXFLAGS=-Wno-shadow -Wno-effc++ -Wno-redundant-decls -Wno-missing-noreturn
.flex.so:
	flex -o `basename $@ .so`.cpp $<  
	$(CXXCOMPILE) $(FLEX_CXXFLAGS) -shared -fPIC $(CXXFLAGS) -o $@ `basename $@ .so`.cpp 
