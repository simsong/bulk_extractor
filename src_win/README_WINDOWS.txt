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
and a CONFIGURE_F17.sh script for configuring a Linux system for
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

2 - On Ubuntu, pthreads is available as  libpthreads-mingw-w64


