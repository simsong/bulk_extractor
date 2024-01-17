#!/bin/bash
# cd to the directory where the script is
# http://stackoverflow.com/questions/59895/can-a-bash-script-tell-what-directory-its-stored-in
OS_NAME=ubuntu
OS_VERSION=22
MAKE_CONCURRENCY=-j2
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd $SCRIPT_DIR

. ./paths.bash


if [ ! -r /etc/os-release ]; then
  echo This requires /etc/os-release
  exit 1
fi
. /etc/os-release
if [ $ID != $OS_NAME ]; then
    echo This requires $OS_NAME Linux. You have $ID.
    exit 1
fi

if [ $VERSION_ID -ne $OS_VERSION ]; then
    echo This requires $OS_NAME version $OS_VERSION. You have $ID $VERSION_ID.
    exit 1
fi
cat <<EOF
*******************************************************************
        Configuring $OS_NAME $OS_VERSION for cross-compiling multi-threaded
		 64-bit Windows programs with mingw.
*******************************************************************

press any key to continue...
EOF
read

MPKGS="autoconf automake flex g++ gcc git libtool libabsl-dev libre2-dev libxml2-utils libz-mingw-w64-dev libgcrypt-mingw-w64-dev libsqlite3-dev make mingw-w64 wine  "

sudo apt update -y
sudo apt install -y $MPKGS
if [ $? != 0 ]; then
  echo "Could not install some of the packages. Will not proceed."
  exit 1
fi

exit 0

echo Attempting to install both DLL and static version of all mingw libraries
echo needed for bulk_extractor.
echo At this point we will keep going even if there is an error...
INST=""
# For these install both DLL and static
# note that liblightgrep needs boost
for lib in libz ; do
  INST+=" ${lib}-mingw-w64 ${lib}-mingw-w64-dev"
done
sudo apt -y install $INST 2>&1 | grep -v "is already installed"

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
