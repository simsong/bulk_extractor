# NOTE 1: This Makefile does not define a primary, so source files are manually included
# using EXTRA_DIST.
#
# NOTE 2: This Makefile is configured in the parent directory, but as part of building
# the installer the makefile does a 'make distclean' in the parent directory and makes
# *two* configured sub directories:
#
#   win32/   - 32-bit windows and Java GUI
#   win64/   - 64-bit windows and Java GUI
# 
# It then builds both of the subdirectories sequentially, then makes the installer
#
# NOTE 3: This makefile is not automatically configured. If it were, then 'make distclean' 
# would wipe it.

BEVERSION=`grep ^AC_INIT ../configure.ac | awk -F, '{print $$2;}' | sed 's/ //'`

INSTALLER=bulk_extractor-$(BEVERSION)-windowsinstaller.exe
INTERMEDIATE_INSTALLER=bulk_extractor-$(BEVERSION)-windowsinstaller_intermediate.exe
SIGNED_INSTALLER=bulk_extractor-$(BEVERSION)-windowsinstaller_signed.exe

UNVERSIONED_INSTALLER=bulk_extractor-windowsinstaller.exe
UNVERSIONED_INTERMEDIATE_INSTALLER=bulk_extractor-windowsinstaller-intermediate.exe
UNVERSIONED_SIGNED_INSTALLER=bulk_extractor-windowsinstaller_signed.exe

WIN32_EXE = bulk_extractor32.exe
WIN64_EXE = bulk_extractor64.exe
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
	$(SRC_WIN)icu4c-53_1-simpler-crossbuild.patch \
	$(SRC_WIN)sign_be_file


# The prerequisites for the installer can't be listed after the colon, 
# becuase then the system will try to generate them simultaneously,
# which causes problems if both try to do a distclean at the same time.
# So this just manually checks to see if the prerequists are present and, if not, it makes them
#
$(UNVERSIONED_INSTALLER): BEViewerLauncher.exe build_installer.nsi EnvVarUpdate.nsi $(PDFDOCS) bulk_extractor64.exe bulk_extractor32.exe BEViewer.jar
	@echo Making bulk_extractor version $(BEVERSION) Windows Installer
	@echo 
	makensis -DVERSION=$(BEVERSION) build_installer.nsi
	@echo 
	echo '**************** INSTALLER IS MADE ****************'
	cp $(INSTALLER) $(UNVERSIONED_INSTALLER)
	ls -lt *exe

$(SRC_WIN)bulk_extractor64.exe:
	if [ ! -r ../build_win64/Makefile ]; then \
	   if [ -r ../Makefile ]; then (cd ..;make distclean); fi ; \
	   if [ ! -d ../build_win64 ]; then mkdir ../build_win64 ; fi; \
	   if [ ! -r ../build_win64/Makefile ]; then (cd ../build_win64;mingw64-configure); fi; \
	fi
	@echo
	echo '**** running make in build_win64/ to create bulk_extractor64.exe ****'; \
	(cd ../build_win64;$(MAKE))
	cp ../build_win64/src/bulk_extractor.exe bulk_extractor64.exe
	cp ../build_win64/java_gui/BEViewer.jar BEViewer.jar


$(SRC_WIN)BEViewer.jar:
	if [ ! -r ../build_java/Makefile ]; then \
	   if [ -r ../Makefile ]; then (cd ..;make distclean); fi ; \
	   if [ ! -d ../build_java ]; then mkdir ../build_java ; fi; \
	   if [ ! -r ../build_java/Makefile ]; then (cd ../build_java;mingw64-configure); fi; \
	fi
	@echo
	echo '**** running make in build_java/ to create bulk_extractor64.exe ****'; \
	(cd ../build_java/java_gui;$(MAKE))
	cp ../build_java/java_gui/BEViewer.jar BEViewer.jar


$(SRC_WIN)bulk_extractor32.exe:
	if [ ! -r ../build_win32/Makefile ]; then \
	   if [ -r ../Makefile ]; then (cd ..;make distclean); fi ; \
	   if [ ! -d ../build_win32 ]; then mkdir ../build_win32 ; fi; \
	   if [ ! -r ../build_win32/Makefile ]; then (cd ../build_win32;mingw32-configure); fi; \
	fi
	@echo
	echo '**** running make in build_win32/ to create bulk_extractor32.exe ****'; \
	(cd ../build_win32;$(MAKE))
	cp ../build_win32/src/bulk_extractor.exe bulk_extractor32.exe

BEViewerLauncher.exe: build_BEViewer_launcher.nsi
	makensis build_BEViewer_launcher.nsi

signed: $(SIGNED_INSTALLER)

$(SRC_WIN)clean:
	@echo '**** MAKE CLEAN ****'
	/bin/rm -rf ../build_win32 ../build_win64  ../build_java 
	/bin/rm -f $(WIN32_EXE) $(WIN64_EXE) $(JAVA_GUI) BEViewerLauncher.exe
	/bin/rm -f $(INSTALLER) $(INTERMEDIATE_INSTALLER) $(SIGNED_INSTALLER) 
	/bin/rm -f $(UNVERSIONED_INSTALLER) $(UNVERSIONED_INTERMEDIATE_INSTALLER) $(UNVERSIONED_SIGNED_INSTALLER)
	/bin/rm -f $(PDFDOCS)

$(SRC_WIN)cleanpdf:
	/bin/rm -f $(PDFDOCS)

$(SRC_WIN)distclean:
	if [ -r ../Makefile ]; then (cd ..;$(MAKE) distclean); fi
	$(MAKE) clean

$(SRC_WIN)release:
	scp $(INSTALLER) $(WIN32_EXE) $(WIN64_EXE) ncr.nps.edu:/var/www/digitalcorpora/downloads/bulk_extractor/

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

################################################################
##
## Everything that follows is for the signed installer, which we are no longer doing
##

$(UNVERSIONED_SIGNED_INSTALLER): $(INTERMEDIATE_INSTALLER)
	./sign_be_file $< $@
	cp $(SIGNED_INSTALLER) bulk_extractor-$(BEVERSION)-windowsinstaller-signed.exe

$(UNVERSIONED_INTERMEDIATE_INSTALLER): $(INSTALLER) \
		signed_bulk_extractor32.exe signed_bulk_extractor64.exe \
		signed_BEViewerLauncher.exe signed_uninstall.exe
	makensis -DVERSION=$(VERSION) -DSIGN build_installer.nsi
	echo '**************** INTERMEDIATE INSTALLER IS MADE ****************'
	ls -l $(INTERMEDIATE_INSTALLER)

signed_bulk_extractor32.exe: bulk_extractor32.exe
	./sign_be_file $< $@

signed_bulk_extractor64.exe: bulk_extractor64.exe
	./sign_be_file $< $@

signed_BEViewerLauncher.exe: BEViewerLauncher.exe
	./sign_be_file $< $@

# Note: file launcher.exe is imported from a Windows system where it is installed unsigned.
signed_uninstall.exe: uninstall.exe
	./sign_be_file $< $@

uninstall.exe: $(INSTALLER)
	@echo 'Error: The required uninstaller file "uninstall.exe" is not present or is old.'
	@echo "Please create this file and copy it into this directory as follows:"
	@echo '  1) Create the unsigned installer by typing "make".'
	@echo "  2) Run the unsigned installer on a Windows system.  This action will"
	@echo "     additionally install the uninstaller."
	@echo "  3) Copy the uninstaller from the Windows system back into this directory."
	@echo "     It should be at path \c:\Program Files (x86)\Bulk Extractor <version>\uninstall.exe."
	@echo 'If the system clock on your Windows system is slower, you may need to "touch uninstall.exe"'
	@echo "after it is installed in this directory."
	@echo ''
	@echo 'In the future, this will be made with WINE'
	exit 1

#.PHONY: signed

# windist makes bulk_extractor32.exe and bulk_extractor64.exe
# and puts them in a zip file. Most users should make the installer instead
# We don't use this anymore, but we might in the future if we want to add the txt files to the installer
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

SUFFIXES = .txt .1
.1.txt:
	/usr/bin/tbl $< | /usr/bin/groff -S -Wall -mtty-char -mandoc -Tascii | /usr/bin/col -bx > $@


