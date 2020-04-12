#!/bin/bash
cat <<EOF
*******************************************************************
        Configuring Ubuntu 16.04 LTS to compile bulk_extractor.
*******************************************************************

Install Ubuntu 16.04 LTS and follow these commands:

# apt-get install git
# git clone --recursive https://github.com/simsong/bulk_extractor.git
# cd bulk_extractor
# bash etc/CONFIGURE_UBUNTU16LTS.bash
# bash bootstrap.sh
# ./configure
# make && sudo make install

press any key to continue...
EOF
read

# cd to the directory where the script is
# http://stackoverflow.com/questions/59895/can-a-bash-script-tell-what-directory-its-stored-in
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
if [ "$PWD" != "$DIR" ]; then
    changed_dir="true"
else
    changed_dir="false"
fi
cd $DIR

#MPKGS="autoconf automake flex gcc git libtool "
#MPKGS+="openssl patch wget bison g++ libssl-dev zlib1g-dev libxml2-dev"
MKPGS="autoconf automake flex gcc g++ libssl-dev zlib1g-dev libxml2-dev dpkg-dev openssl patch wget bison"

if [ ! -r /etc/os-release ]; then
    echo This requires Debian Linux.
    exit 1
fi

source /etc/os-release

if [ x$ID != xubuntu ]; then
    echo This really requires ubuntu. You have $ID
    exit 1
fi
MAJOR_VERSION=`echo $VERSION_ID|sed s/[.].*//`
if [ $MAJOR_VERSION -ne 16 ]; then
    echo This requires Ubuntu 16 LTS Linux.
    exit 1
fi


echo Will now try to install 

# I use emacs. Installing it may install requiremnts
sudo apt update -y
sudo apt install -y emacs
sudo apt autoremove -y autoremove
# Now install what is required

echo apt install -y $MKPGS
sudo apt install -y $MKPGS 
exit 1
if [ $? != 0 ]; then
  echo "Could not install some of the packages. Will not proceed."
  exit 1
fi

exit 0

# ICU requires patching and a special build sequence
#

echo "Building and installing ICU for mingw"
ICUVER=53_1
ICUFILE=icu4c-$ICUVER-src.tgz
ICUDIR=icu
ICUURL=http://download.icu-project.org/files/icu4c/53.1/$ICUFILE

if is_installed libicuuc
then
  echo ICU is already installed
else
  if [ ! -r $ICUFILE ]; then
    wget $ICUURL
  fi
  tar xf $ICUFILE

  # patch ICU for MinGW cross-compilation
  pushd icu
  patch -p0 <../icu4c-53_1-simpler-crossbuild.patch
  patch -p0 <../icu4c-53_1-mingw-w64-mkdir-compatibility.patch
  popd

  ICUDIR=`tar tf $ICUFILE|head -1`

  ICU_DEFINES="-DU_USING_ICU_NAMESPACE=0 -DU_CHARSET_IS_UTF8=1 -DUNISTR_FROM_CHAR_EXPLICIT=explicit -DUNSTR_FROM_STRING_EXPLICIT=explicit"

  ICU_FLAGS="--disable-extras --disable-icuio --disable-layout --disable-samples --disable-tests"

  # build ICU for Linux to get packaging tools used by MinGW builds
  echo
  echo icu linux
  rm -rf icu-linux
  mkdir icu-linux
  pushd icu-linux
  CC=gcc CXX=g++ CFLAGS=-O3 CXXFLAGS=-O3 CPPFLAGS="$ICU_DEFINES" ../icu/source/runConfigureICU Linux --enable-shared $ICU_FLAGS
  make VERBOSE=1
  popd
  
  # build 64-bit ICU for MinGW
  echo
  echo icu mingw64
  rm -rf icu-mingw64
  mkdir icu-mingw64
  pushd icu-mingw64
  eval MINGW=\$MINGW64
  eval MINGW_DIR=\$MINGW64_DIR
  ../icu/source/configure CC=$MINGW-gcc CXX=$MINGW-g++ CFLAGS=-O3 CXXFLAGS=-O3 CPPFLAGS="$ICU_DEFINES" --enable-static --disable-shared --prefix=$MINGW_DIR --host=$MINGW --with-cross-build=`realpath ../icu-linux` $ICU_FLAGS --disable-tools --disable-dyload --with-data-packaging=static
  make VERBOSE=1
  sudo make install
  make clean
  popd
  rm -rf icu-mingw64
  rm -rf $ICUDIR icu-linux
  echo "ICU mingw installation complete."
fi

#
# build liblightgrep
#

build_mingw liblightgrep  https://github.com/LightboxTech/liblightgrep/archive/v1.3.0.tar.gz  liblightgrep-1.3.0.tar.gz


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
echo 'You are now ready to cross-compile for win64.'
echo 'To make bulk_extractor64.exe: cd ..; make win64'
echo 'To make ZIP file with both:   cd ..; make windist'
echo 'To make the Nulsoft installer with both and the Java GUI: make'
if [ "$changed_dir" == "true" ]; then
    echo "NOTE: paths are relative to the directory $0 is in"
fi
