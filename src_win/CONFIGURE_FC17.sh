#!/bin/sh
cat <<EOF
*******************************************************************
  Configuring Fedora for cross-compiling multi-threaded 32-bit and
		 64-bit Windows programs with mingw.
*******************************************************************

This script will configure a fresh Fedora system to compile with
mingw32 and 64.  Please perform the following steps:

1. Install F17 or newer, running with you as an administrator.
   For a VM:

   1a - download the ISO for the 64-bit DVD (not the live media) from:
        http://fedoraproject.org/en/get-fedora-options#formats
   1b - Create a new VM using this ISO as the boot.
       
2. Plese put this CONFIGURE_FC17.sh script in you home directory.

3. Run this script to configure the system to cross-compile bulk_extractor.
   This script must be run as root.  You can do that by typing:
          sudo sh CONFIGURE_F17.sh

press any key to continue...
EOF
read

MPKGS="autoconf automake gcc gcc-c++ osslsigncode mingw32-nsis flex wine wget"
MPKGS+="mingw32-gcc mingw32-gcc-c++ mingw32-zlib mingw32-zlib-static mingw32-libgnurx-static "
MPKGS+="mingw64-gcc mingw64-gcc-c++ mingw64-zlib mingw64-zlib-static mingw64-libgnurx-static "

echo "Command do execute:"
echo $MPKGS

if [ $USER != "root" ]; then
  echo This script must be run as root
  exit 1
fi

if [ ! -r /etc/redhat-release ]; then
  echo This requires Fedora Linux
  exit 1
fi

if grep 'Fedora.release.' /etc/redhat-release ; then
  echo Fedora Release detected
else
  echo This script is only tested for Fedora Release 17 and should work on FC17 or newer.
  exit 1
fi

if [ $USER != "root" ]; then
  echo This script must be run as root
  exit 1
fi

echo Will now try to install 

yum install -y $MPKGS
if [ $? != 0 ]; then
  echo "Could not install some of the packages. Will not proceed."
  exit 1
fi

echo 
echo "Now performing a yum update to update system packages"
echo yum -y update

PTHREADS_URL="https://github.com/downloads/simsong/bulk_extractor/pthreads-w32-2-9-1-release.tar.gz"

for CROSS in i686-w64-mingw32 x86_64-w64-mingw32 ; do
  echo Checking pthreads for $CROSS
  DESTDIR=/usr/$CROSS/sys-root/mingw/include/
  if [ ! -r $DESTDIR/pthread.h ]; then
    if [ ! -r pthreads-w32-2-9-1-release.tar.gz ]; then
	echo Getting pthreads from $PTHREADS_URL
	wget $PTHREADS_URL
    fi
    if [ ! -r pthreads-w32-2-9-1-release/Makefile ]; then
        /bin/rm -rf pthreads-w32-2-9-1-release
        tar xfvz pthreads-w32-2-9-1-release.tar.gz
    fi
    pushd pthreads-w32-2-9-1-release
      make CROSS=$CROSS- CFLAGS="-DHAVE_STRUCT_TIMESPEC -I." clean GC-static
      install implement.h need_errno.h pthread.h sched.h semaphore.h $DESTDIR
      if [ $? != 0 ]; then
        echo "Unable to install include files for $CROSS"
        exit 1
      fi
      install *.a /usr/$CROSS/sys-root/mingw/lib/
      if [ $? != 0 ]; then
        echo "Unable to install library for $CROSS"
        exit 1
      fi
      make clean
    popd
  fi
done

echo "Building and installing TRE for mingw"
wget http://laurikari.net/tre/tre-0.8.0.zip
unzip tre-0.8.0.zip
cd tre-0.8.0
mingw32-configure --enable-static
make
sudo make install
mingw64-configure --enable-static
make
sudo make install
cd ..
rm tre-0.8.0.zip
rm -rf tre-0.8.0
echo "TRE mingw installation complete."

echo "Building and installing LIBEWF for mingw"
wget http://libewf.googlecode.com/files/libewf-experimental-20120809.tar.gz
tar -zxf libewf-experimental-20120809.tar.gz
cd libewf-20120809
mingw32-configure --enable-static
make
sudo make install
mingw64-configure --enable-static
make
sudo make install
cd ..
rm libewf-experimental-20120809.tar.gz
rm -rf libewf-20120809
echo "LIBEWF mingw installation complete."

echo ================================================================
echo ================================================================
echo You are now ready to cross-compile for win32 and win64.
echo 'You may be able to do this by typing "make win32" or "make win64"'
echo 'You can also type "mingw64-configure && make"'
echo
echo "Please additionally install the recommended LIBEWF and TRE"
echo 'libraries by running "./CONFIGURE_LIBRARIES.sh"'.
