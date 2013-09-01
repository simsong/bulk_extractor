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

INSTALLER = bulk_extractor-$(VERSION)-windowsinstaller.exe
INTERMEDIATE_INSTALLER = bulk_extractor-$(VERSION)-windowsinstaller-intermediate.exe
SIGNED_INSTALLER = bulk_extractor-$(VERSION)-windowsinstaller-signed.exe

EXTRA_DIST = \
	build_BEViewer_launcher.nsi \
	build_installer.nsi \
	CONFIGURE_F18.bash \
	icu-mingw32-libprefix.patch \
	icu-mingw64-libprefix.patch \
	README.txt \
	README_WINDOWS.txt \
	sign_be_file

all: $(INSTALLER)

signed: $(SIGNED_INSTALLER)

# The prerequisites for the installer can't be listed after the colon, 
# becuase then the system will try to generate them simultaneously,
# and make win32 can't be run at the same time as make win64
# also, win32 and win64 do a makedistclean, so no targets can be made after they are made
# So this just manually checks to see if the prerequists are present and, if not, it makes them
#
WIN32_EXE = win32/bulk_extractor32.exe
WIN64_EXE = win64/bulk_extractor64.exe
JAVA_GUI  = win64/java_gui/BEViewer.jar
$(INSTALLER): BEViewerLauncher.exe build_installer.nsi
	if [ ! -r $(WIN32_EXE) ]; then \
	   (cd ..;make distclean); \
	   (rm -rf win32;mkdir win32;cd win32;mingw32-configure;make;cp src/bulk_extractor.exe bulk_extractor32.exe);\
           fi
	if [ ! -r $(WIN64_EXE) ]; then \
	   (cd ..;make distclean); \
	   (rm -rf win64;mkdir win64;cd win64;mingw64-configure;make;cp src/bulk_extractor.exe bulk_extractor64.exe);\
	   fi
	if [ ! -r $(JAVA_GUI) ]; then \
	   (cd ..;make distclean); \
	   (rm -rf win64;mkdir win64;cd win64;mingw64-configure;make;cp src/bulk_extractor.exe bulk_extractor64.exe);\
	   fi
	makensis -DVERSION=$(VERSION) build_installer.nsi
	echo '**************** INSTALLER IS MADE ****************'
	ls -l $(INSTALLER)

BEViewerLauncher.exe: build_BEViewer_launcher.nsi
	makensis build_BEViewer_launcher.nsi

################################################################
##
## Everything that follows is for the signed installer, which we are no longer doing
##

$(SIGNED_INSTALLER): $(INTERMEDIATE_INSTALLER)
	./sign_be_file $< $@

$(INTERMEDIATE_INSTALLER): $(INSTALLER) \
		signed_bulk_extractor32.exe signed_bulk_extractor64.exe \
		signed_BEViewerLauncher.exe signed_uninstall.exe
	makensis -DVERSION=$(VERSION) -DSIGN build_installer.nsi
	echo '**************** INTERMEDIATE INSTALLER IS MADE ****************'
	ls -l $(INTERMEDIATE_INSTALLER)

signed_bulk_extractor32.exe: $(WIN32_EXE)
	./sign_be_file $< $@

signed_bulk_extractor64.exe: $(WIN64_EXE)
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

.PHONY: signed

