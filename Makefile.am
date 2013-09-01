# bulk_extractor Makefile.am
# This file is compiled with automake to create Makefile.in. 
# Makefile.in is transformed by "configure" to create Makefile
#

if WANT_BE_VIEWER
BE_VIEWER_DIR = java_gui
endif

if WANT_WIN_INSTALLER
SRC_WIN_DIR = src_win
endif

SUBDIRS = doc doc/user_manual src man plugins python specfiles tests $(BE_VIEWER_DIR) $(SRC_WIN_DIR)


# don't include bootstrap. People run it, and they shoudln't
# It's only for people who check out the git repo

EXTRA_DIST = \
	ROADMAP.txt \
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
	m4/ax_boost_base.m4 \
	m4/ax_boost_filesystem.m4 \
	m4/ax_boost_system.m4 \
	m4/ax_pthread.m4

ACLOCAL_AMFLAGS = -I m4

.PHONY: exels
exels:
	/bin/ls -l */*exe

.PHONY: execlean
execlean:
	/bin/rm -f */*exe

.PHONY: win32
win32: win32/bulk_extractor32.exe

win32/bulk_extractor32.exe:
	@echo '****************************************************************'
	@echo '*** make win32 with a distclean, mingw32-configure, and make ***'
	@echo '****************************************************************'
	rm -rf win32
	make distclean
	(mkdir win32;cd win32;mingw32-configure;make;cp src/bulk_extractor.exe bulk_extractor32.exe)
	ls -l win32/bulk_extractor32.exe

.PHONY: win64
win64: win64/bulk_extractor64.exe

win64/bulk_extractor64.exe:
	@echo '****************************************************************'
	@echo '*** make win64 with a distclean, mingw32-configure, and make ***'
	@echo '****************************************************************'
	rm -rf win64
	make distclean
	(mkdir win64;cd win64;mingw64-configure;make;cp src/bulk_extractor.exe bulk_extractor64.exe)
	ls -l win64/bulk_extractor64.exe

# windist makes bulk_extractor32.exe and bulk_extractor64.exe
# and puts them in a zip file
.PHONY: windist
windist: win32/bulk_extractor32.exe win64/bulk_extractor64.exe
	@echo checking to see if there are uncommitted sources
	(if (svn status | grep '^Q') ; then exit 1 ; fi)
	@echo nope
	rm -rf $(distdir).zip $(distdir) src/*.exe
	mkdir $(distdir)
	mkdir $(distdir)/python
	@echo
	cp win32/bulk_extractor32.exe $(distdir)
	cp win64/bulk_extractor64.exe $(distdir)
	@echo ====================================
	@echo 
	@echo making documentation
	make man/bulk_extractor.txt
	mv man/bulk_extractor.txt $(distdir)
	@echo ====================================
	@echo 
	@echo Creating ZIP archive
	@echo 
	zip -r9 $(distdir).zip $(distdir)
	@echo ====================================
	@echo 
	@echo Adding text files to $(distdir).zip 
	@echo 
	cp python/*.{py,txt}    $(distdir)/python
	cp ChangeLog		$(distdir)/ChangeLog.txt
	cp NEWS			$(distdir)/NEWS.txt
	cp COPYING		$(distdir)/COPYING.txt
	md5deep -r  $(distdir) > md5list.txt
	md5deep -rd $(distdir) > md5list.xml
	mv md5list.txt md5list.xml $(distdir)
	zip --to-crlf $(distdir).zip $(distdir)/*.txt $(distdir)/*.xml $(distdir)/python/*
	rm -rf $(distdir) 
	@echo "***********************"
	@echo "*** WINDIST IS MADE ***"
	@echo "***********************"
	@echo ""
	ls -l $(distdir).*
	@echo ""
	@unzip -l $(distdir).zip

.PHONY: winrelease
winrelease: $(BEZIP)
	@echo checking to see if there are uncommitted sources
	(if (svn status | grep '^M') ; then exit 1 ; fi)
	@echo nope
	echo make $(distdir).zip
	scp $(BEZIP) $(RELEASE_SSH)
	ssh $(RELEASE_USER)$(RELEASE_HOST) 'echo $(BEZIP) > $(RELEASE_LOC)/$(BEREL)'

.PHONY: svnclean
svnclean:
	echo these files will be deleted: 
	svn status | grep '^[?]' | awk '{print $2;}'
	echo 
	echo hit return to continue
	read
	/bin/rm -rf `svn status | grep '^[?]' | awk '{print $2;}'`

.PHONY: pull
pull:
	git pull
	(cd src/dfxml;git pull)
	(cd src/be13_api;git pull)

CLEANFILES = man/bulk_extractor.txt

SUFFIXES = .txt .1
.1.txt:
	/usr/bin/tbl $< | /usr/bin/groff -S -Wall -mtty-char -mandoc -Tascii | /usr/bin/col -bx > $@


