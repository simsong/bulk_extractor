#!/bin/bash
RELEASE=20
CONFIGURE="./configure -q --enable-silent-rules"
LIBEWF_DIST=https://github.com/libyal/libewf-legacy/releases/download/20140812/libewf-20140812.tar.gz
AUTOCONF_DIST=https://ftp.gnu.org/gnu/autoconf/autoconf-2.71.tar.gz
AUTOMAKE_DIST=https://ftp.gnu.org/gnu/automake/automake-1.16.3.tar.gz
MKPGS="autoconf automake build-essential flex libexpat1-dev libssl-dev libtool libxml2-utils make pkg-config zlib1g-dev"
WGET="wget -nv --no-check-certificate"
CONFIGURE="./configure -q --enable-silent-rules"
MAKE="make -j4"
trap "exit 1" TERM
export TOP_PID=$$
cat <<EOF
*******************************************************************
        Configuring Ubuntu $RELEASE.04 LTS to compile bulk_extractor.
*******************************************************************

Install Ubuntu $RELEASE.04 and follow these commands:

# sudo apt-get install git
# git clone --recursive https://github.com/simsong/bulk_extractor.git
# bash bulk_extractor/etc/CONFIGURE_UBUNTU20LTS.bash
# (cd bulk_extractor; sudo make install)

press any key to continue...
EOF
read IGNORE

function fail() {
    echo FAIL: $@
    kill -s TERM $TOP_PID
}

# cd to the directory where the script is
# http://stackoverflow.com/questions/59895/can-a-bash-script-tell-what-directory-its-stored-in
DIR="$(readlink -f $(dirname "${BASH_SOURCE[0]}"))"
cd /tmp
/bin/rm -rf src
mkdir src
cd src

if [ ! -r /etc/os-release ]; then
    echo This requires an /etc/os-release file.
    exit 1
fi

source /etc/os-release

if [ x$ID != xubuntu ]; then
    echo This really requires ubuntu. You have $ID
    exit 1
fi

MAJOR_VERSION=`echo $VERSION_ID|sed s/[.].*//`
if [ $MAJOR_VERSION -lt $RELEASE ]; then
    echo This requires at least Ubuntu $RELEASE Linux.
    exit 1
fi

echo Will now try to install

sudo apt upgrade -y || fail could not apt upgrade
sudo apt install -y $MKPGS || fail could not apt install $MKPGS

echo manually installing a modern libewf
$WGET $LIBEWF_DIST || (echo could not download $LIBEWF_DIST; exit 1)
tar xfz libewf*gz   && (cd libewf*/   && $CONFIGURE && $MAKE >/dev/null && sudo make install)
ls -l /etc/ld.so.conf.d/
sudo ldconfig
ewfinfo -h >/dev/null || (echo could not install libewf; exit 1)

echo updating autoconf
$WGET $AUTOCONF_DIST || (echo could not download $AUTOCONF_DIST; exit 1)
tar xfz autoconf*gz && (cd autoconf*/ && $CONFIGURE && $MAKE >/dev/null && sudo make install)
autoconf --version

echo updating automake
$WGET $AUTOMAKE_DIST || (echo could not download $AUTOMAKE_DIST; exit 1)
tar xfz automake*gz && (cd automake*/ && $CONFIGURE && $MAKE >/dev/null && sudo make install)
automake --version
