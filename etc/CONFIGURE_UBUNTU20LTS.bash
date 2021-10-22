#!/bin/bash
RELEASE=20
LIBEWF_DIST=https://github.com/libyal/libewf-legacy/releases/download/20140812/libewf-20140812.tar.gz
AUTOCONF_DIST=https://ftpmirror.gnu.org/autoconf/autoconf-2.71.tar.gz
AUTOMAKE_DIST=https://ftpmirror.gnu.org/automake/automake-1.16.3.tar.gz
MKPGS="libtool autoconf automake libssl-dev pkg-config libxml2-utils"
WGET="wget -nv --no-check-certificate"
cat <<EOF
*******************************************************************
        Configuring Ubuntu $RELEASE.04 LTS to compile bulk_extractor.
*******************************************************************

Install Ubuntu $RELEASE.04 and follow these commands:

# apt-get install git
# git clone --recursive https://github.com/simsong/bulk_extractor.git
# cd bulk_extractor
# bash etc/CONFIGURE_UBUNTU$RELEASE.bash
# ./ bootstrap.sh  && ./configure && make && make check && sudo make install

press any key to continue...
EOF
read IGNORE

# cd to the directory where the script is
# http://stackoverflow.com/questions/59895/can-a-bash-script-tell-what-directory-its-stored-in
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
if [ "$PWD" != "$DIR" ]; then
    changed_dir="true"
else
    changed_dir="false"
fi
cd $DIR

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

sudo apt update -y
sudo apt install -y $MKPGS

CONFIGURE="./configure -q --enable-silent-rules"
echo manually installing a modern libewf
$WGET $LIBEWF_DIST || (echo could not download $LIBEWF_DIST; exit 1)
tar xfz libewf*gz   && (cd libewf*/   && $CONFIGURE && make && sudo make install)
ls -l /etc/ld.so.conf.d/
sudo ldconfig

echo updating autoconf
$WGET $AUTOCONF_DIST || (echo could not download $AUTOCONF_DIST; exit 1)
tar xfz autoconf*gz && (cd autoconf*/ && $CONFIGURE && make && sudo make install)
autoconf --version

echo updating automake
$WGET $AUTOMAKE_DIST || (echo could not download $AUTOMAKE_DIST; exit 1)
tar xfz automake*gz && (cd automake*/ && $CONFIGURE && make && sudo make install)
automake --version
