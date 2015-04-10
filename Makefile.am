# bulk_extractor Makefile.am
# This file is compiled with automake to create Makefile.in. 
# Makefile.in is transformed by "configure" to create Makefile
#

# Bring in the defines from src_win (mostly SRC_WIN_DIST)
SRC_WIN=src_win/
include src_win/Makefile


# These two conditionals are set by configure. If they, 
# they cause a "make" in the top-level to automatically run make in the subdirs
if WANT_BE_VIEWER
BE_VIEWER_DIR = java_gui
endif

## Note: to build the windows installer you must go into the directory,
## since it is not using automake
## if WANT_WIN_INSTALLER
## SRC_WIN_DIR=src_win
## endif

SUBDIRS = doc doc/latex_manuals src man plugins python specfiles tests $(BE_VIEWER_DIR) 

# don't include bootstrap. People run it, and they shoudln't
# It's only for people who check out the git repo

EXTRA_DIST = \
	m4/ac_check_classpath.m4 \
	m4/ac_check_junit.m4 \
	m4/ac_check_rqrd_class.m4 \
	m4/ac_java_options.m4 \
	m4/ac_prog_jar.m4 \
	m4/ac_prog_java.m4 \
	m4/ac_prog_java_cc.m4 \
	m4/ac_prog_java_works.m4 \
	m4/ac_prog_javac.m4 \
	m4/ac_prog_javac_works.m4 \
	m4/ac_prog_javadoc.m4 \
	m4/ac_prog_javah.m4 \
	m4/ac_try_compile_java.m4 \
	m4/ac_try_run_javac.m4 \
	m4/ax_pthread.m4 \
	bootstrap.sh \
	$(SRC_WIN_DIST)

ACLOCAL_AMFLAGS = -I m4

.PHONY: exels
exels:
	/bin/ls -l */*exe

.PHONY: execlean
execlean:
	/bin/rm -f */*exe

.PHONY: win32
win32: win32/bulk_extractor32.exe

.PHONY: win64
win64: win64/bulk_extractor64.exe

win32/bulk_extractor32.exe:
	@echo '****************************************************'
	@echo '*** making win32 with mingw32-configure and make ***'
	@echo '****************************************************'
	if [ -r Makefile ]; then $(MAKE) distclean ; fi
	if [ ! -d win32  ]; then mkdir win32 ; fi
	if [ ! -r win32/Makefile ]; then (cd win32;mingw32-configure) ; fi
	(cd win32;$(MAKE))
	(cp win32/src/bulk_extractor.exe win32/bulk_extractor32.exe)
	ls -l win32/bulk_extractor32.exe

win64/bulk_extractor64.exe:
	@echo '****************************************************'
	@echo '*** making win64 with mingw32-configure and make ***'
	@echo '****************************************************'
	if [ -r Makefile ]; then $(MAKE) distclean ; fi
	if [ ! -d win64  ]; then mkdir win64 ; fi
	if [ ! -r win64/Makefile ]; then (cd win64;mingw64-configure) ; fi
	(cd win64;$(MAKE))
	(cp win64/src/bulk_extractor.exe win64/bulk_extractor64.exe)
	ls -l win64/bulk_extractor64.exe

winlibs:
	@echo '*****************************************'
	@echo '*** making win32 and win64 libraries  ***'
	@echo '*****************************************'
	if [ -r Makefile ]; then $(MAKE) distclean ; fi
	if [ ! -d win32  ]; then mkdir win32 ; fi
	if [ ! -r win32/Makefile ]; then (cd win32;mingw32-configure --disable-libewf --disable-afflib) ; fi
	(cd win32/src;$(MAKE) lib;mv libbulkextractor.so ../../libbulkextractor32.DLL)	
	if [ ! -d win64  ]; then mkdir win64 ; fi
	if [ ! -r win64/Makefile ]; then (cd win64;mingw64-configure --disable-libewf --disable-afflib) ; fi
	(cd win64/src;$(MAKE) lib;mv libbulkextractor.so ../../libbulkextractor64.DLL)	
	ls -l libbulk*.DLL


libs-with-sceadan:
	@echo '************************************************'
	@echo '*** make all exes and libraries with sceadan ***'
	@echo '************************************************'
	if [ ! -r ../sceadan ]; then echo sceadan must be installed at ../sceadan ; exit 1 ; fi
	/bin/rm -rf linux win32 win64
	make distclean
	@echo Making Linux
	(cd ../sceadan;make clean;sh configure;make)
	mkdir linux
	(cd linux;../configure --with-sceadan=../../sceadan --disable-afflib --disable-libewf;cd src;make bulk_extractor libbulkextractor.so)
	@echo Making win32
	(cd ../sceadan;make clean;mingw32-configure;make)
	mkdir win32
	(cd win32;mingw32-configure --with-sceadan=../../sceadan --disable-afflib --disable-libewf;cd src;make bulk_extractor.exe libbulkextractor.so)
	@echo Making win64
	(cd ../sceadan;make clean;mingw64-configure;make)
	mkdir win64
	(cd win64;mingw64-configure --with-sceadan=../../sceadan --disable-afflib --disable-libewf;cd src;make bulk_extractor.exe libbulkextractor.so)


.PHONY: lib
lib:
	(cd src; $(MAKE) lib)

.PHONY: pull
pull:
	git pull
	(cd src/dfxml;git checkout master; git pull)
	(cd src/be13_api;git checkout master; git pull)

.PHONY: publish
publish:
	$(MAKE) dist
	scp $(DIST_ARCHIVES) ncr.nps.edu:/var/www/digitalcorpora/downloads/bulk_extractor

gitfixup:
	(cd src/be13_api/;git remote set-url origin git@github.com:/simsong/be13_api.git;git checkout master)
	(cd src/dfxml/;git remote set-url origin git@github.com:/simsong/dfxml.git; git checkout master)

.PHONY: libinstall
libinstall:
	install -D src/libbulkextractor.so $(prefix)/lib/libbulkextractor.so
	install -D python/module/bulkextractor.py $(prefix)/lib/python2.7/dist-packages/bulkextractor.py
	if [ -r /sbin/ldconfig ]; /sbin/ldconfig -v ; fi
