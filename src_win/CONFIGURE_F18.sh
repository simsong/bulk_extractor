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

MPKGS="autoconf automake gcc gcc-c++ osslsigncode flex wine zlib-devel wget md5deep git "
MPKGS+="mingw32-gcc mingw32-gcc-c++ "
MPKGS+="mingw64-gcc mingw64-gcc-c++ "

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

echo Attempting to install both DLL and static version of all mingw libraries
echo At this point we will keep going even if there is an error...
for M in mingw32 mingw64 ; do
  for lib in zlib gettext boost cairo pixman freetype fontconfig bzip2 expat pthreads libgnurx tre wpcap nsis ; do
    echo ${M}-${lib} ${M}-${lib}-static
  done
done | xargs sudo yum -y install


echo 
echo "Now performing a yum update to update system packages"
sudo yum -y update


MINGW32=i686-w64-mingw32
MINGW64=x86_64-w64-mingw32

MINGW32_DIR=/usr/$MINGW32/sys-root/mingw
MINGW64_DIR=/usr/$MINGW64/sys-root/mingw

# from here on, exit if any command fails
set -e

#
# TRE
#

echo "Building and installing TRE for mingw"
TREVER=0.8.0
TREFILE=tre-$TREVER.tar.gz
TREDIR=tre-$TREVER
TREURL=http://laurikari.net/tre/$TREFILE

wget $TREURL
tar xfvz $TREFILE
pushd tre-$TREVER
for i in 32 64 ; do
  echo
  echo libtre mingw$i
  mingw$i-configure --enable-static --disable-shared
  make
  sudo make install
  make clean
done
popd
echo "TRE mingw installation complete."

#
# EWF
#

echo "Building and installing LIBEWF for mingw"
EWFVER=20130128
EWFFILE=libewf-$EWFVER.tar.gz
EWFDIR=libewf-$EWFVER
EWFURL=http://libewf.googlecode.com/files/$EWFFILE

wget $EWFURL 
tar xzf $EWFFILE 
pushd $EWFDIR
for i in 32 64 ; do
  echo
  echo libewf mingw$i
  mingw$i-configure --enable-static --disable-shared
  make
  sudo make install
  make clean
done
popd
echo "LIBEWF mingw installation complete."

#
# ICU
#

echo "Building and installing ICU for mingw"
ICUVER=50_1_2
ICUFILE=icu4c-$ICUVER-src.tgz
ICUDIR=icu
ICUURL=http://download.icu-project.org/files/icu4c/50.1.2/$ICUFILE

wget $ICUURL
tar xzf $ICUFILE
patch -p1 <icu-mingw-libprefix.patch
echo
echo icu linux
mkdir icu-linux
pushd icu-linux
CC=gcc CXX=g++ CFLAGS=-O3 CXXFLAGS=-O3 CPPFLAGS="-DU_USING_ICU_NAMESPACE=0 -DU_CHARSET_IS_UTF8=1 -DUNISTR_FROM_CHAR_EXPLICIT=explicit -DUNSTR_FROM_STRING_EXPLICIT=explicit" ../icu/source/runConfigureICU Linux --enable-shared --disable-extras --disable-icuio --disable-layout --disable-samples
make
popd
echo
echo icu mingw32
mkdir icu-mingw32
pushd icu-mingw32
../icu/source/configure CC=$MINGW32-gcc CXX=$MINGW32-g++ CFLAGS=-O3 CXXFLAGS=-O3 CPPFLAGS="-DU_USING_ICU_NAMESPACE=0 -DU_CHARSET_IS_UTF8=1 -DUNISTR_FROM_CHAR_EXPLICIT=explicit -DUNSTR_FROM_STRING_EXPLICIT=explicit" --enable-static --disable-shared --prefix=$MINGW32_DIR --host=$MINGW32 --with-cross-build=`realpath ../icu-linux` --disable-extras --disable-icuio --disable-layout --disable-samples --with-data-packaging=static --disable-dyload
make
sudo make install
popd
echo
echo icu mingw64
mkdir icu-mingw64
pushd icu-mingw64
../icu/source/configure CC=$MINGW64-gcc CXX=$MINGW64-g++ CFLAGS=-O3 CXXFLAGS=-O3 CPPFLAGS="-DU_USING_ICU_NAMESPACE=0 -DU_CHARSET_IS_UTF8=1 -DUNISTR_FROM_CHAR_EXPLICIT=explicit -DUNSTR_FROM_STRING_EXPLICIT=explicit" --enable-static --disable-shared --prefix=$MINGW64_DIR --host=$MINGW64 --with-cross-build=`realpath ../icu-linux` --disable-extras --disable-icuio --disable-layout --disable-samples --with-data-packaging=static --disable-dyload
make
sudo make install
popd
echo "ICU mingw installation complete."

#
# Lightgrep
#

echo "Building and installing lightgrep for mingw"
#LGVER=
#LGFILE=
LGDIR=liblightgrep
LGURL=https://github.com/jonstewart/liblightgrep.git

git clone --recursive $LGURL $LGDIR
pushd $LGDIR
#echo
#echo lightgrep mingw32
#make clean
#sudo make install
echo
echo lightgrep mingw64
make CROSS=1
sudo make CROSS=1 install
popd

#
#
#

echo "Cleaning up"
rm -f $TREFILE $EWFFILE $ICUFILE
rm -rf $TREDIR $EWFDIR icu icu-linux icu-mingw32 icu-mingw64 $LGDIR

echo ...
echo 'Now running ../bootstrap.sh and configure'
pushd ..
sh bootstrap.sh
sh configure
popd
echo ================================================================
echo ================================================================
echo 'You are now ready to cross-compile for win32 and win64.'
echo 'To make bulk_extractor32.exe: cd ..; make win32'
echo 'To make bulk_extractor64.exe: cd ..; make win64'
echo 'To make ZIP file with both:   cd ..; make windist'
echo 'To make the Nulsoft installer with both and the Java GUI: make'
