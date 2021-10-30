#!/bin/bash

RELEASE=20
REQUIRED_ID='amzn'
REQUIRED_VERSION=2
CONFIGURE="./configure -q --enable-silent-rules"
LIBEWF_DIST=https://github.com/libyal/libewf-legacy/releases/download/20140812/libewf-20140812.tar.gz
AUTOCONF_DIST=https://ftpmirror.gnu.org/autoconf/autoconf-2.71.tar.gz
AUTOMAKE_DIST=https://ftpmirror.gnu.org/automake/automake-1.16.3.tar.gz
MKPGS="autoconf automake libexpat1-dev libssl-dev libtool libxml2-utils pkg-config"
WGET="wget -nv --no-check-certificate"
CONFIGURE="./configure -q --enable-silent-rules"
MAKE="make -j4"
NAME='AWS Linux'
cat <<EOF
*******************************************************************
Configuring, compile and check this bulk_extractor release for $NAME
*******************************************************************

Install AWS Linux and follow these commands:

#
# sudo yum -y update && sudo yum -y install git && git clone --recursive https://github.com/simsong/bulk_extractor.git 
# bash bulk_extractor/etc/CONFIGURE_AMAZON_LINUX.bash
# cd bulk_extractor && make && sudo make install

press any key to continue...
EOF
read IGNORE

# cd to the directory where the script is becuase we will do downloads into tmp
# http://stackoverflow.com/questions/59895/can-a-bash-script-tell-what-directory-its-stored-in
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
mkdir -p $DIR/tmp
cd $DIR/tmp

if [ ! -r /etc/os-release ]; then
  echo This requires Amazon Linux
  exit 1
fi

. /etc/os-release
if [ $ID != $REQUIRED_ID ]; then
  echo This requires operating system ID $REQUIRED_ID
  exit 1
fi

if [ $VERSION != $REQUIRED_VERSION ]; then
    echo This requires operating system VERSION $REQUIRED_VERSION
    exit 1
fi

MPKGS="autoconf automake flex gcc10-c++ git libtool md5deep wget zlib-devel "
MPKGS+="libewf libewf-devel java-1.8.0-openjdk-devel "
MPKGS+="libxml2-devel libxml2-static openssl-devel "
MPKGS+="sqlite-devel expat-devel "
MPKGS+="libjson-c-devel "

echo Will now try to install

sudo yum install -y $MPKGS --skip-broken
if [ $? != 0 ]; then
  echo "Could not install some of the packages. Will not proceed."
  exit 1
fi

echo
echo "Now performing a yum update to update system packages"
sudo yum -y update

echo manually installing a modern libewf
$WGET $LIBEWF_DIST || (echo could not download $LIBEWF_DIST; exit 1)
tar xfz libewf*gz   && (cd libewf*/   && $CONFIGURE && $MAKE >/dev/null && sudo make install)
ls -l /etc/ld.so.conf.d/
sudo ldconfig

echo updating autoconf
$WGET $AUTOCONF_DIST || (echo could not download $AUTOCONF_DIST; exit 1)
tar xfz autoconf*gz && (cd autoconf*/ && $CONFIGURE && $MAKE >/dev/null && sudo make install)
autoconf --version

echo updating automake
$WGET $AUTOMAKE_DIST || (echo could not download $AUTOMAKE_DIST; exit 1)
tar xfz automake*gz && (cd automake*/ && $CONFIGURE && $MAKE >/dev/null && sudo make install)
automake --version

# AWS Linux doesn't set this up by default
echo /usr/local/lib > /etc/ld.so.conf.d/libewf.conf
sudo ldconfig

cd $DIR
CC=gcc10-gcc CXX=gcc10-c++ ./configure && make check
