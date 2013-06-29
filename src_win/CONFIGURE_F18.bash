#!/bin/bash
cat <<EOF
*******************************************************************
  Configuring Fedora for cross-compiling multi-threaded 32-bit and
		 64-bit Windows programs with mingw.
*******************************************************************

This script will configure a fresh Fedora system to compile with
mingw32 and 64.  Please perform the following steps:

1. Install F18 or newer, running with you as an administrator.
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

# cd to the directory where the script it
# http://stackoverflow.com/questions/59895/can-a-bash-script-tell-what-directory-its-stored-in
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd $DIR

NEEDED_FILES=" icu-mingw32-libprefix.patch icu-mingw64-libprefix.patch"
for i in $NEEDED_FILES ; do
  if [ ! -r $i ]; then
    echo This script requires the file $i which is distributed with $0
    exit 1
  fi
done

MPKGS="autoconf automake flex gcc gcc-c++ git libtool "
MPKGS+="md5deep osslsigncode patch wine wget bison zlib-devel "
MPKGS+="libewf libewf-devel "
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
INST=""
for M in mingw32 mingw64 ; do
  # For these install both DLL and static
  for lib in zlib gettext boost cairo pixman freetype fontconfig bzip2 expat pthreads libgnurx ; do
    INST+=" ${M}-${lib} ${M}-${lib}-static"
  done
done 
sudo yum -y install $INST

echo 
echo "Now performing a yum update to update system packages"
sudo yum -y update

MINGW32=i686-w64-mingw32
MINGW64=x86_64-w64-mingw32

MINGW32_DIR=/usr/$MINGW32/sys-root/mingw
MINGW64_DIR=/usr/$MINGW64/sys-root/mingw

# from here on, exit if any command fails
set -e

function is_installed {
  LIB=$1
  if [ -r /usr/x86_64-w64-mingw32/sys-root/mingw/lib/$LIB.a ] && \
     [ -r /usr/i686-w64-mingw32/sys-root/mingw/lib/$LIB.a ];
  then
    return 0
  else 
    return 1
  fi
}
    
function build_mingw {
  LIB=$1
  URL=$2
  FILE=$3
  if is_installed $LIB
  then
    echo $LIB already installed. Skipping
  else
    echo Building $1 from $URL/$FILE
    if [ ! -r $FILE ]; then
       wget $URL/$FILE
    fi
    tar xfvz $FILE
    # Now get the directory that it unpacked into
    DIR=`tar tfz $FILE |head -1`
    pushd $DIR
    for i in 32 64 ; do
      echo
      echo %%% $LIB mingw$i
      mingw$i-configure --enable-static --disable-shared
      make
      sudo make install
      make clean
    done
    popd
    rm -rf $DIR
  fi
}

build_mingw libtre   http://laurikari.net/tre/   tre-0.8.0.tar.gz
build_mingw libewf   https://googledrive.com/host/0B3fBvzttpiiSMTdoaVExWWNsRjg   libewf-20130416.tar.gz

#
# ICU requires patching and a special build sequence
#

echo "Building and installing ICU for mingw"
ICUVER=51_1
ICUFILE=icu4c-$ICUVER-src.tgz
ICUDIR=icu
ICUURL=http://download.icu-project.org/files/icu4c/51.1/$ICUFILE

if is_installed libsicuuc
then
  echo ICU is already installed
else
  if [ ! -r $ICUFILE ]; then
    wget $ICUURL
  fi
  tar xzf $ICUFILE
  patch -p1 < icu-mingw32-libprefix.patch
  patch -p1 < icu-mingw64-libprefix.patch
  
  ICUDIR=`tar tfz $ICUFILE|head -1`
  # build ICU for Linux to get packaging tools used by MinGW builds
  echo
  echo icu linux
  mkdir icu-linux
  pushd icu-linux
  CC=gcc CXX=g++ CFLAGS=-O3 CXXFLAGS=-O3 CPPFLAGS="-DU_USING_ICU_NAMESPACE=0 -DU_CHARSET_IS_UTF8=1 -DUNISTR_                                        FROM_CHAR_EXPLICIT=explicit -DUNSTR_FROM_STRING_EXPLICIT=explicit" ../icu/source/runConfigureICU Linux --enable-shared --disable-extras --disable-icuio --disable-layout --disable-samples --disable-tests
  make VERBOSE=1
  popd
  rm -rf icu-linux
  
  # build 32- and 64-bit ICU for MinGW
  for i in 32 64 ; do
    echo
    echo icu mingw$i
    mkdir icu-mingw$i
    pushd icu-mingw$i
    eval MINGW=\$MINGW$i
    eval MINGW_DIR=\$MINGW${i}_DIR
    ../icu/source/configure CC=$MINGW-gcc CXX=$MINGW-g++ CFLAGS=-O3 CXXFLAGS=-O3 CPPFLAGS="-DU_USING_ICU_NAMESPACE=0 -DU_CHARSET_IS_UTF8=1 -DUNISTR_FROM_CHAR_EXPLICIT=explicit -DUNSTR_FROM_STRING_EXPLICIT=explicit" --enable-static --disable-shared --prefix=$MINGW_DIR --host=$MINGW --with-cross-build=`realpath ../icu-linux` --disable-extras --disable-icuio --disable-layout --disable-samples --disable-tests --with-data-packaging=static --disable-dyload
    make VERBOSE=1
    sudo make install
    make clean
    popd
    rm -rf icu-mingw$i
  done
  rm -rf $ICUDIR
  echo "ICU mingw installation complete."
fi

#
# Lightgrep is currently built from github, which I don't like
#

if is_installed liblightgrep
then
  echo liblightgrep is already installed
else
  echo "Building and installing lightgrep for mingw"
  LGDIR=liblightgrep
  LGURL=git://github.com/LightboxTech/liblightgrep.git
  
  git clone --recursive $LGURL $LGDIR
  pushd $LGDIR
  autoreconf -i
  for i in 32 64 ; do
    echo
    echo liblightgrep mingw$i
    mingw$i-configure --enable-static --disable-shared
    make
    sudo make install
    make clean
  done
  popd
  echo "liblightgrep mingw installation complete."
  rm -rf $LGDIR
fi

#
#
#


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
