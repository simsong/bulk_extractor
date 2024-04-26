#!/bin/bash
MYDIR=$(dirname $(readlink -f $0))
cd $MYDIR
source paths.bash
OS_NAME=fedora
OS_VERSION=39
MPKGS="autoconf automake make flex gcc gcc-c++ git libtool wget zlib-devel "
MPKGS+="java-1.8.0-openjdk-devel libxml2-devel libxml2-static openssl-devel "
MPKGS+="sqlite-devel expat-devel "

# from here on, exit if any command fails
set -e


if [ ! -r /etc/os-release ]; then
  echo This requires /etc/os-release
  exit 1
fi
. /etc/os-release
if [ "$ID" != 'fedora' ]; then
    echo This requires $OS_NAME Linux. You have $ID.
    exit 1
fi

if [ $VERSION_ID -ne $OS_VERSION ]; then
    echo This requires $OS_NAME version $OS_VERSION. You have $ID $VERSION_ID.
    exit 1
fi

sudo yum install -y $MPKGS --skip-broken
if [ $? != 0 ]; then
  echo "Could not install some of the packages. Will not proceed."
  exit 1
fi

echo
echo "Now performing a yum update to update system packages"
sudo yum -y update

if [ -r  /usr/local/lib/libewf.a ]; then
    echo libewf is already installed
else
    echo
    echo "Now installing libewf into $LIBEWF_DIR"
    wget -nv $LIBEWF_URL  || (echo could not download $LIBEWF_URL. Stop; exit 1)
    tar xfz $LIBEWF_FNAME || (echo could not untar $LIBEWF_FNAME. Stop; exit 1)
    (cd $LIBEWF_DIR  \
	 && ./configure --quiet --enable-silent-rules --prefix=/usr/local \
	 && make \
	 && sudo make install) || (echo could not build libewf. Stop; exit 1)
    echo Cleaning up $LIBEWF_FNAME and $LIBEWF_DIR
    /bin/rm -rf $LIBEWF_FNAME $LIBEWF_DIR || (echo could not clean up. Stop; exit 1)
fi

# Make sure that /usr/local/lib is in ldconfig
sudo /bin/rm -f /tmp/local.conf
echo /usr/local/lib > /tmp/local.conf
sudo mv /tmp/local.conf /etc/ld.so.conf.d/local.conf
sudo ldconfig

#
#
#

echo ================================================================
echo ================================================================
echo 'You are now ready to compile bulk_extractor for Linux.'
echo 'To compile, be sure you are in the root directory and type:'
echo ''
echo '    sh bootstrap.sh && ./configure && make'
echo ''
echo 'To make a distribution, type make release'
