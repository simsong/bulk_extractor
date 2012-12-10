This file contains information on compiling bulk_extractor for Windows.

Currently there are two approaches:

  1 - You can cross-compile for Windows using mingw32 or mingw64 on Fedora Core,
      please see src_win/README.
  2 - You can compile natively on Windows using mingw32

In order to use mingw as a cross-compiler, the following libraries
must be installed in the Mingw library directories:

  1 - pthreads for Windows
  2 - zlib
  3 - (optional) libewf (for handling EnCase files)
  4 - (optional) libaff and openssl (for handling AFF files) 

All of these require various kinds of flags. 

We now have a build environment that does almost all of this for you,
and a CONFIGURE_FC17.sh script for configuring a Linux system for
cross-compiling, located in this directory.

To cross-compile each of the additional libraries, use:

$ mingw32-configure ; make clean && make && sudo make install
$ mingw64-configure ; make clean && make && sudo make install

For example for E01 support, cross-compile libewf as follows:
  1 - download libewf-20120813.tar.gz
  2 - uncompress and cd to libewf-20120813
  3 - mingw32-configure
  4 - make && sudo make install && make clean
  5 - mingw64-configure
  6 - make && sudo make install && make clean

*** NOTE NOTE NOTE:
# http://www.mingw.org/wiki/GCCStatus
All modules must be linked with -shared-libgcc to allow exceptions to be thrown across DLL boundaries.
This is an issue with libexiv2

================
Compiling natively under Windows with MINGW:
*******************************************

  Download the Windows Server 2003 Resource Kit tools from:
  http://www.microsoft.com/downloads/details.aspx?familyid=9d467a69-57ff-4ae7-96ee-b18c4790cffd&displaylang=en

  download and run mingw-get-inst-20101030.exe (or whatever version is current),
  selecting all options including these:

    C Compiler, C++ Compiler. MSYS Basic System, MinGW Development Toolkit.

  When selecting the installation path to MinGW, Do not define a path
  with spaces in it.

  Start the MinGW32 shell window.

  Download the latest repository catalog and update and install
  modules required by MinGW by typing the following:

  mingw-get update
  mingw-get install g++
  mingw-get install pthreads
  mingw-get install mingw32-make
  mingw-get install zlib
  mingw-get install libz-dev

  Install the libraries required by bulk_extractor and also install bulk_extractor in this order:
    * expat
    * openssl
    * libewf  (be sure to configure --enable-winapi=yes)
    * afflib
    * regex
    * bulk_extractor

  For each library:
   - download
   - ./configure --prefix=/usr/local/ --enable-winapi=yes
   - make
   - make install

   For openssl, run "./config --prefix=/usr/local" rather than configure.

   Don't make directories in your home directory if there is a space in it! 
   Libtool doesn't handle paths with spaces in them.

  If OpenSSL is installed in /usr/local/ssl, you may need to build
  other libraries with:  

  ./configure CPPFLAGS="-I/usr/local/include" -I/usr/local/ssl/include" \
              LDFLAGS="-L/usr/local/lib -L/usr/local/ssl/lib"

  Most libraries will install in /usr/local/ ; you may need to add
  -I/usr/local/include to CFLAGS and -L/usr/local/lib to your make
  scripts

  Still problematic, though, is actually running what is
  produced. Unless you link -static you will have a lot of DLL
  references. Most of the DLLs are installed in /usr/local/bin/*.dll
  and /bin/*.dll and elsewhere, which maps typically to
  c:\mingw\msys\1.0\local\bin and c:\mingw\bin\

Compiling natively on Windows using cygwin (untested):
*****************************************************

Cygwin:
 * Go to cygwin.org. Download and run Setup.
 * install these modules:
   g++ 4.0
   autoconf
   automake
   libexif-devel
   libopenssl098
   subversion
   openssh

