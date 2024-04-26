#!/bin/bash

OS_NAME=fedora
OS_VERSION=39
USE_ICU=NO
MAKE_CONCURRENCY=-j2
MPKGS="bison osslsigncode patch wine mingw64-gcc mingw64-gcc-c++ mingw32-nsis "
MINGW_PKGS="zlib gettext boost cairo pixman freetype fontconfig bzip2 expat winpthreads libgnurx libxml2 iconv openssl sqlite"

# cd to the directory where the script is
# http://stackoverflow.com/questions/59895/can-a-bash-script-tell-what-directory-its-stored-in

MYDIR=$(dirname $(readlink -f $0))
source $MYDIR/paths.bash
bash $MYDIR/CONFIGURE_FEDORA${OS_VERSION}.bash

if [ $USE_ICU == "YES" ]; then
    NEEDED_FILES="icu4c-53_1-mingw-w64-mkdir-compatibility.patch"
    NEEDED_FILES+=" icu4c-53_1-simpler-crossbuild.patch"
    for i in $NEEDED_FILES ; do
	if [ ! -r $i ]; then
	    echo This script requires the file $i which is distributed with $0
	    exit 1
	fi
    done

    #
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
fi

echo Will now try to install

sudo yum install -y $MPKGS --skip-broken 2>&1 | grep -v "is already installed"
if [ $? != 0 ]; then
  echo "Could not install some of the packages. Will not proceed."
  exit 1
fi

echo Attempting to install both DLL and static version of all mingw libraries
echo needed for bulk_extractor.
echo At this point we will keep going even if there is an error...
INST=""
# For these install both DLL and static
# note that liblightgrep needs boost
for lib in $MINGW_PKGS ; do
  INST+=" mingw64-${lib} mingw64-${lib}-static"
done
sudo yum -y install $INST 2>&1 | grep -v "is already installed"

echo
echo "Now performing a yum update to update system packages"
sudo yum -y update  2>&1 | grep -v "is already installed"

MINGW64=x86_64-w64-mingw32
MINGW64_DIR=/usr/$MINGW64/sys-root/mingw

# from here on, exit if any command fails
set -e

function is_installed {
  LIB=$1
  if [ -r /usr/x86_64-w64-mingw32/sys-root/mingw/lib/$LIB.a ];
  then
    return 0
  else
    return 1
  fi
}

# usage: build_mingw <name> <download-URL>
function build_mingw {
    LIB=$1
    URL=$2
    FILE=`basename $2`
    if is_installed $LIB; then
	echo $LIB already installed. Skipping
    else
	echo Building $LIB from $URL from $FILE.
	if [ ! -r $FILE ]; then
	    echo wget --content-disposition $URL
	    wget --content-disposition $URL
	fi
	if [ ! -r $FILE ]; then
	    echo $FILE did not download from $URL.
	    exit 1
	fi
	tar xvf $FILE
	# Now get the directory that it unpacked into
	DIR=`tar tf $FILE |head -1`
	pushd $DIR
	echo
	echo %%% $LIB mingw64
	if [ ! -r configure -a -r bootstrap.sh ]; then
	    . bootstrap.sh
	fi
	CPPFLAGS=-DHAVE_LOCAL_LIBEWF mingw64-configure --enable-static --disable-shared
	make clean
	make $MAKE_CONCURRENCY
	sudo make install
	make clean
	popd
	rm -rf $DIR
    fi
}

build_mingw libtre   http://laurikari.net/tre/tre-0.8.0.tar.gz
build_mingw libewf   $LIBEWF_URL


#
# build liblightgrep
# --- currently disabled, as there is no lightgrep release at https://github.com/strozfriedberg/lightgrep/releases
# build_mingw liblightgrep  https://github.com/LightboxTech/liblightgrep/archive/v1.3.0.tar.gz  liblightgrep-1.3.0.tar.gz


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
