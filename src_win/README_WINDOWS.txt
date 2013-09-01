This file contains information on compiling bulk_extractor for Windows.

Currently there is only one approach that is tested and approved for compiling bulk_extractor for Windows:

  1 - You can cross-compile for Windows using mingw32 or mingw64 with
  the Fedora operating system.

There are other approaches for compiling bulk_extractor for windows
that we have used in the past, but they are no longer supported. Those
approaches were:

  x - Compiling bulk_extractor natively on windows using mingw
  x - Cross-compiling bulk_extractor using mingw on Ubuntu
  x - Cross-compiling bulk_extractor using mingw on MacOS



1. PREPARATION
==============

In order to use mingw as a cross-compiler, the following libraries
must be installed in the Mingw library directories:

  1 - pthreads for Windows
  2 - zlib
  3 - (optional) libewf (for handling EnCase files)
  4 - (optional) libaff and openssl (for handling AFF files) 
  5 - (optional) liblightgrep (for Lightgrep)
  6 - (optional) hashdb (for the sector hash)

In general, to create the cross-compiled versions of the above
library, you do the following

  - Download the library
  - cd into the library's base directory
  $ ( mkdir win32 ; cd win32 ; mingw32-configure && make && sudo make install)
  $ ( mkdir win64 ; cd win64 ; mingw64-configure && make && sudo make install)

The bash script CONFIGURE_F18.bash will do all of this for you and
more, for each library, as well as download the necessary patches.

To use this script:

 - Download the Fedora 18 DVD
 - Create an empty Virtual Machine, boot it with this DVD image, and
   install the OS onto the VM
 - Install VMWare tools
 $ yum install wget
 $ wget https://raw.github.com/simsong/bulk_extractor/master/src_win/CONFIGURE_F18.bash
 $ sudo bash CONFIGURE_F18.bash
  
2. BUILDING BULK_EXTRACTOR INSTALLER FOR WINDOWS
================================================

 - Download the bulk_extractor installer or clone the git repository and run setup
 - cd src_win
 - make

NOTE: the Makefile in the src_win directory no longer uses automake,
becuase it needs to do multiple automakes and reconfigurations to
build the Java GUI, the 32-bit windows version, and the 64-bit windows version.

The Makefile will also create an unsigned Windows installer.



3. SIGNING THE INSTALLER
========================

If you wish to create a signed Windows installer:

Signing tool "osslsigncode" for Authenticode signing of EXE/CAB files is required.
Please install package osslsigncode.

Please define and export these Environment variables in order to sign files:
Environment variable FOUO_BE_CERT must point to the code signing certificate.
Environment variable FOUO_BE_CERT_PASSWORD must contain the password for the code signing certificate.

File uninstall.exe must also be present so that it, too, can be signed.
Unfortunately, obtaining it is convoluted:
    1) Create the unsigned installer by typing "make", as described above.
    2) Run the unsigned installer on a Windows system.  This action will
       additionally install the uninstaller.
    3) Copy the uninstaller from the Windows system back into this directory.
       It should be at path \c:\Program Files (x86)\Bulk Extractor <version>\uninstall.exe.
If the system clock on your Windows system is slower, you may need to "touch uninstall.exe"
after it is installed in this directory.

Now, run "make signed".

