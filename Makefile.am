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

# these are for the DFRWS2013 challenge:
#	src/data_sniffer src/dfrws_2012_challenge.py


ACLOCAL_AMFLAGS = -I m4

RELEASE_USER = simsong@
RELEASE_HOST = www.digitalcorpora.org
RELEASE_DIR  = digitalcorpora.org/

RELEASE_LOC  = $(RELEASE_DIR)/downloads/
VERSION_FN   = $(PACKAGE)_version.txt
RELEASE_PATH = $(RELEASE_LOC)/$(VERSION_FN)
RELEASE_SSH  = $(RELEASE_USER)$(RELEASE_HOST):$(RELEASE_LOC)
RELEASE	     = $(PACKAGE)-$(VERSION)

release:
	make check_release_version
	make dist
	make distcheck
	make the_release

check_release_version:
	@echo Checking version on server for $(VERSION_FN)
	@echo
	echo
	/bin/rm -f $(VERSION_FN)
	wget -q http://$(RELEASE_HOST)/downloads/$(VERSION_FN)
	@echo Version `cat $(VERSION_FN)` is on the server.
	@sh -c "if [ `cat $(VERSION_FN)` = $(RELEASE).tar.gz ]; then echo ; echo ; echo $(RELEASE) is already on the server; exit 1; fi"
	/bin/rm -f $(VERSION_FN)

the_release: 
	gpg --detach-sign $(RELEASE).tar.gz
	scp $(RELEASE).tar.gz{,.sig} $(RELEASE_SSH)
	ssh $(RELEASE_HOST) 'cd $(RELEASE_LOC);/bin/rm $(PACKAGE).tar.gz;ln -s $(RELEASE).tar.gz $(PACKAGE).tar.gz'
	ssh $(RELEASE_HOST) 'echo $(RELEASE).tar.gz > $(RELEASE_PATH)'
	@echo Release $(RELEASE) uploaded to server

win32:
	rm -rf win32
	mkdir win32
	make distclean
	mingw32-configure
	make
	cp src/bulk_extractor.exe win32/bulk_extractor32.exe

win64:
	rm -rf win64
	mkdir win64
	make distclean
	mingw64-configure
	make
	cp src/bulk_extractor.exe win64/bulk_extractor64.exe

BEREL=bulk_extractor-windows_version.txt
BEZIP=bulk_extractor-windows-$(VERSION).zip

$(distdir).zip: 
	make windist

# windist makes bulk_extractor32.exe and bulk_extractor64.exe
# and puts them in a zip file
windist: win32 win64
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
	rm -rf $(distdir) $(WINDOWSDOCS) 
	@echo "***********************"
	@echo "*** WINDIST IS MADE ***"
	@echo "***********************"
	@echo ""
	ls -l $(distdir).*
	@echo ""
	@unzip -l $(distdir).zip

$(BEZIP): $(distdir).zip
	mv $(distdir).zip $(BEZIP)

winrelease: $(BEZIP)
	@echo checking to see if there are uncommitted sources
	(if (svn status | grep '^M') ; then exit 1 ; fi)
	@echo nope
	echo make $(distdir).zip
	scp $(BEZIP) $(RELEASE_SSH)
	ssh $(RELEASE_USER)$(RELEASE_HOST) 'echo $(BEZIP) > $(RELEASE_LOC)/$(BEREL)'

svnclean:
	echo these files will be deleted: 
	svn status | grep '^[?]' | awk '{print $2;}'
	echo 
	echo hit return to continue
	read
	/bin/rm -rf `svn status | grep '^[?]' | awk '{print $2;}'`

pull:
	git pull
	(cd src/dfxml;git pull)
	(cd src/be13_api;git pull)

CLEANFILES = man/bulk_extractor.txt

SUFFIXES = .txt .1
.1.txt:
	/usr/bin/tbl $< | /usr/bin/groff -S -Wall -mtty-char -mandoc -Tascii | /usr/bin/col -bx > $@

.PHONY: windist win32 win64

