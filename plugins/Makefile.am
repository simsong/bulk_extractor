#
# make bulk_extractor plugins
# stand

AM_CPPFLAGS =  -Ibe13_api 

bin_PROGRAMS = demo 

demo_SOURCES = demo.cpp

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
