# Standalone makefile, not made with autotools.
#
# NOTE 1: This Makefile does not define a primary, so source files are manually included
# using EXTRA_DIST.
#
# NOTE 2: This Makefile is configured in the parent directory, but as part of building
# the installer the makefile does a 'make distclean' in the parent directory and makes
# *two* configured sub directories:
#
#   win64/   - 64-bit windows and Java GUI
# 
# It then builds both of the subdirectories sequentially, then makes the installer
#
# NOTE 3: This makefile is not automatically configured. If it were, then 'make distclean' 
# would wipe it.

BEVERSION=`grep ^AC_INIT ../configure.ac | awk -F, '{print $$2;}' | sed 's/ //'`

INSTALLER=bulk_extractor-$(BEVERSION)-windowsinstaller.exe

UNVERSIONED_INSTALLER=bulk_extractor-windowsinstaller.exe

WIN64_EXE = bulk_extractor.exe
JAVA_GUI  = BEViewer.jar

PDFDOCS	  = BEProgrammersManual.pdf BEUsersManual.pdf BEWorkedExamplesStandalone.pdf

SRC_WIN_DIST = \
	$(SRC_WIN)CONFIGURE_F20.bash \
	$(SRC_WIN)EnvVarUpdate.nsi \
	$(SRC_WIN)Makefile \
	$(SRC_WIN)README_WINDOWS.txt \
	$(SRC_WIN)build_BEViewer_launcher.nsi \
	$(SRC_WIN)build_installer.nsi \
	$(SRC_WIN)icu4c-53_1-mingw-w64-mkdir-compatibility.patch \
	$(SRC_WIN)icu4c-53_1-simpler-crossbuild.patch

zipfile: 
	zip bulk_extractor-$(BEVERSION).zip $(SRC_WIN)bulk_extractor.exe bulk_extractor-$(BEVERSION)-windowsinstaller.exe 

# The prerequisites for the installer can't be listed after the colon, 
# becuase then the system will try to generate them simultaneously,
# which causes problems if both try to do a distclean at the same time.
# So this just manually checks to see if the prerequists are present and, if not, it makes them
#
$(UNVERSIONED_INSTALLER): BEViewerLauncher.exe build_installer.nsi EnvVarUpdate.nsi $(PDFDOCS) bulk_extractor.exe BEViewer.jar
	@echo Making bulk_extractor version $(BEVERSION) Windows Installer
	@echo 
	makensis -DVERSION=$(BEVERSION) build_installer.nsi
	@echo 
	echo '**************** INSTALLER IS MADE ****************'
	cp $(INSTALLER) $(UNVERSIONED_INSTALLER)
	ls -lt *exe

$(SRC_WIN)bulk_extractor.exe:
	if [ ! -r ../build_win64/Makefile ]; then \
	   if [ -r ../Makefile ]; then (cd ..;make distclean); fi ; \
	   if [ ! -d ../build_win64 ]; then mkdir ../build_win64 ; fi; \
	   if [ ! -r ../build_win64/Makefile ]; then (cd ../build_win64;mingw64-configure); fi; \
	fi
	@echo
	echo '**** running make in build_win64/ to create bulk_extractor.exe ****'; \
	(cd ../build_win64;$(MAKE))
	cp ../build_win64/src/bulk_extractor.exe bulk_extractor.exe
	cp ../build_win64/java_gui/BEViewer.jar BEViewer.jar


$(SRC_WIN)BEViewer.jar:
	if [ ! -r ../build_java/Makefile ]; then \
	   if [ -r ../Makefile ]; then (cd ..;make distclean); fi ; \
	   if [ ! -d ../build_java ]; then mkdir ../build_java ; fi; \
	   if [ ! -r ../build_java/Makefile ]; then (cd ../build_java;mingw64-configure); fi; \
	fi
	@echo
	echo '**** running make in build_java/ to create BEViewer.jar ****'; \
	(cd ../build_java/java_gui;$(MAKE))
	cp ../build_java/java_gui/BEViewer.jar BEViewer.jar

BEViewerLauncher.exe: build_BEViewer_launcher.nsi
	makensis build_BEViewer_launcher.nsi

$(SRC_WIN)clean:
	@echo '**** MAKE CLEAN ****'
	/bin/rm -rf ../build_win64  ../build_java 
	/bin/rm -f $(WIN64_EXE) $(JAVA_GUI) BEViewerLauncher.exe
	/bin/rm -f $(INSTALLER)
	/bin/rm -f $(UNVERSIONED_INSTALLER)
	/bin/rm -f $(PDFDOCS)

$(SRC_WIN)cleanpdf:
	/bin/rm -f $(PDFDOCS)

$(SRC_WIN)distclean:
	if [ -r ../Makefile ]; then (cd ..;$(MAKE) distclean); fi
	$(MAKE) clean

$(SRC_WIN)release:
	scp $(INSTALLER) $(WIN64_EXE) cfrs.vse.gmu.edu:/home/dcorpora/downloads/bulk_extractor/

$(SRC_WIN)distdir:
	if [ ! -d src_win ]; then mkdir src_win ; fi


################################################################
## 
## Get the PDF documentation from the download site
##
DOWNLOAD = http://digitalcorpora.org/downloads/bulk_extractor
BEProgrammersManual.pdf:	
	wget $(DOWNLOAD)/BEProgrammersManual.pdf

BEUsersManual.pdf:	
	wget $(DOWNLOAD)/BEUsersManual.pdf

BEWorkedExamplesStandalone.pdf:	
	wget $(DOWNLOAD)/BEWorkedExamplesStandalone.pdf

# windist makes a Windows distribution.
# Most users should make the installer instead
# We don't use this anymore, but we might in the future if we want to add the txt files to the installer
windist: win64/bulk_extractor.exe
	@echo checking to see if there are uncommitted sources
	(if (svn status | grep '^Q') ; then exit 1 ; fi)
	@echo nope
	rm -rf $(distdir).zip $(distdir) src/*.exe
	mkdir $(distdir)
	mkdir $(distdir)/python
	@echo
	cp win64/bulk_extractor.exe $(distdir)
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

SUFFIXES = .txt .1
.1.txt:
	/usr/bin/tbl $< | /usr/bin/groff -S -Wall -mtty-char -mandoc -Tascii | /usr/bin/col -bx > $@


