#!/bin/bash
SCRIPT_DIR="$(readlink -f $(dirname "${BASH_SOURCE[0]}"))"
RELEASE=22
CONFIGURE="./configure -q --enable-silent-rules"
MKPGS="autoconf automake g++ flex libabsl-dev libexpat1-dev libre2-dev libssl-dev libtool libssl-dev libxml2-utils make pkg-config zlib1g-dev"
WGET="wget -nv --no-check-certificate"
CONFIGURE="./configure -q --enable-silent-rules"
MAKE="make -j2"
trap "exit 1" TERM
export TOP_PID=$$
cat <<EOF
*******************************************************************
        Configuring Ubuntu $RELEASE.04 LTS to compile bulk_extractor.
*******************************************************************

Install Ubuntu $RELEASE.04 and follow these commands:

# sudo apt-get install git
# git clone --recursive https://github.com/simsong/bulk_extractor.git
# bash bulk_extractor/etc/CONFIGURE_UBUNTU22LTS.bash
# (cd bulk_extractor; sudo make install)

press any key to continue...
EOF
read IGNORE

. ./paths.bash

function fail() {
    echo FAIL: $@
    kill -s TERM $TOP_PID
}

if [ ! -r /etc/os-release ]; then
    echo == This requires an /etc/os-release file.
    exit 1
fi

source /etc/os-release

if [ x$ID != xubuntu ]; then
    echo == This really requires ubuntu. You have $ID
    exit 1
fi

MAJOR_VERSION=`echo $VERSION_ID|sed s/[.].*//`
if [ $MAJOR_VERSION -lt $RELEASE ]; then
    echo == This requires at least Ubuntu $RELEASE Linux.
    exit 1
fi

echo == Will now try apt update
sudo apt update --fix-missing -y || fail could not apt update

echo == Will now try to install
sudo apt install -y $MKPGS || fail could not apt install $MKPGS

echo == manually installing a modern libewf from $LIBEWF_URL
cd /tmp
/bin/rm -rf src
mkdir src
cd src

$WGET $LIBEWF_URL || (echo could not download $LIBEWF_URL; exit 1)
tar xfz libewf*gz   && (cd libewf*/   && $CONFIGURE && $MAKE >/dev/null && sudo make install)
ls -l /etc/ld.so.conf.d/
sudo ldconfig
ewfinfo -h >/dev/null || (echo could not install libewf; exit 1)
