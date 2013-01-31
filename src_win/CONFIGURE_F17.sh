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
       
2. Plese put this CONFIGURE_F17.sh script in you home directory.

3. Run this script to configure the system to cross-compile bulk_extractor.
   Parts of this script will be run as root using "sudo".

press any key to continue...
EOF
read

MPKGS="autoconf automake gcc gcc-c++ osslsigncode mingw32-nsis flex wine zlib-devel wget md5deep "
MPKGS+="mingw32-gcc mingw32-gcc-c++ mingw32-zlib mingw32-zlib-static mingw32-libgnurx-static "
MPKGS+="mingw64-gcc mingw64-gcc-c++ mingw64-zlib mingw64-zlib-static mingw64-libgnurx-static "

echo "Command do execute:"
echo $MPKGS

if [ ! -r /etc/redhat-release ]; then
  echo This requires Fedora Linux
  exit 1
fi

if grep 'Fedora.release.' /etc/redhat-release ; then
  echo Fedora Release detected
else
  echo This script is only tested for Fedora Release 17 and should work on F17 or newer.
  exit 1
fi

echo Will now try to install 

sudo yum install -y $MPKGS
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
      sudo install implement.h need_errno.h pthread.h sched.h semaphore.h $DESTDIR
      if [ $? != 0 ]; then
        echo "Unable to install include files for $CROSS"
        exit 1
      fi
      sudo install *.a /usr/$CROSS/sys-root/mingw/lib/
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
make distclean
mingw64-configure --enable-static
make
sudo make install
cd ..
echo "TRE mingw installation complete."

echo "Building and installing LIBEWF for mingw"
EWFVER=20130128
wget http://libewf.googlecode.com/files/libewf-$EWFVER.tar.gz
tar xfz libewf-$EWFVER.tar.gz
cd libewf-$EWFVER
echo
echo libewf mingw32
mingw32-configure --enable-static
make
sudo make install
make distclean
echo
echo libewf mingw64
mingw64-configure --enable-static
make
sudo make install
make distclean
cd ..

echo "Cleaning up"
rm tre-0.8.0.zip
rm -rf tre-0.8.0
rm libewf-$EWFVER.tar.gz
rm -rf libewf-$EWFVER
echo "LIBEWF mingw installation complete."
echo ...
echo 'Now running ../bootstrap.sh and configure'
cd ..
sh bootstrap.sh
sh configure > /dev/null
echo ================================================================
echo ================================================================
echo 'You are now ready to cross-compile for win32 and win64.'
echo 'To make bulk_extractor32.exe: cd ..; make win32'
echo 'To make bulk_extractor64.exe: cd ..; make win64'
echo 'To make ZIP file with both:   cd ..; make windist'
echo 'To make the Nulsoft installer with both and the Java GUI: make'
