# NOTE: This Makefile does not define a primary, so source files are manually included
# using EXTRA_DIST.

WIN32_EXE = ../win32/bulk_extractor32.exe
WIN64_EXE = ../win64/bulk_extractor64.exe
JAVA_GUI  = ../java_gui/BEViewer.jar

INSTALLER = be_installer-$(VERSION)_unsigned.exe
INTERMEDIATE_INSTALLER = be_installer-$(VERSION)_intermediate.exe
SIGNED_INSTALLER = be_installer-$(VERSION).exe

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
$(INSTALLER): $(JAVA_GUI) BEViewerLauncher.exe build_installer.nsi
	mv $(JAVA_GUI) /tmp/java_gui$$  # make a copy of it 
	if [ ! -r $(WIN32_EXE) ]; then \
	   (cd ..;make distclean;rm -rf win32;mkdir win32;cd win32;mingw32-configure;make;cp src/bulk_extractor.exe bulk_extractor32.exe);\
           fi
	if [ ! -r $(WIN64_EXE) ]; then \
	    (cd ..;make distclean;rm -rf win64;mkdir win64;cd win64;mingw64-configure;make;cp src/bulk_extractor.exe bulk_extractor64.exe);\
	   fi
	mv /tmp/java_gui$$ $(JAVA_GUI) # move it back
	makensis -DVERSION=$(VERSION) build_installer.nsi

$(JAVA_GUI):
	cd ../java_gui && make

BEViewerLauncher.exe: build_BEViewer_launcher.nsi
	makensis build_BEViewer_launcher.nsi

## Support for creating a signed installer.
#define sign_file =
## certificate and password environment variables must be set to sign .exe files.
#if test -z "$FOUO_BE_CERT"
#then
#    echo "Error: Environment variable FOUO_BE_CERT is not set."
#    echo "Environment variable FOUO_BE_CERT must be set to the code signing certificate file"
#    echo "and environment variable FOUO_BE_CERT_PASSWORD must contain the password for the code signing certificate file."
#    exit 1
#fi
#if test -z "$FOUO_BE_CERT_PASSWORD"
#then
#    echo "Error: Environment variable FOUO_BE_CERT_PASSWORD is not set."
#    echo "Environment variable FOUO_BE_CERT must be set to the code signing certificate file"
#    echo "and environment variable FOUO_BE_CERT_PASSWORD must contain the password for the code signing certificate file."
#    exit 1
#fi
#
#osslsigncode -pkcs12 $FOUO_BE_CERT -pass $FOUO_BE_CERT_PASSWORD -in $^ -out $@
#endef

$(SIGNED_INSTALLER): $(INTERMEDIATE_INSTALLER)
	./sign_be_file $< $@

$(INTERMEDIATE_INSTALLER): $(INSTALLER) \
		signed_bulk_extractor32.exe signed_bulk_extractor64.exe \
		signed_BEViewerLauncher.exe signed_uninstall.exe
	makensis -DVERSION=$(VERSION) -DSIGN build_installer.nsi

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
	exit 1

.PHONY: signed

