#!/bin/bash
LIBEWF_URL=https://github.com/libyal/libewf-legacy/releases/tag/20140814
OS_NAME=fedora
OS_VERSION=36
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

cat <<EOF
*******************************************************************
        Configuring $OS_NAME $OS_VERSION compiling bulk_extractor
*******************************************************************

This script will configure a fresh install to compile
bulk_extractor.  Please perform the following steps:

1. Start a VM
2. git clone https://github.com/simsong/bulk_extractor.git
3. Run this script
4. make && sudo make install

EOF

if [ "$1" != '-nowait' ]; then
    echo press any key to continue...
    read
fi

# cd to the directory where the script is
cd "$( dirname "${BASH_SOURCE[0]}" )"

MPKGS="autoconf automake make flex gcc gcc-c++ git libtool wget zlib-devel "
MPKGS+="java-1.8.0-openjdk-devel libxml2-devel libxml2-static openssl-devel "
MPKGS+="sqlite-devel expat-devel "

echo Will now try to install

sudo yum install -y $MPKGS --skip-broken
if [ $? != 0 ]; then
  echo "Could not install some of the packages. Will not proceed."
  exit 1
fi

echo
echo "Now performing a yum update to update system packages"
sudo yum -y update


LIBEWF_FNAME=$(basename $LIBEWF_URL)
LIBEWF_DIR=$( echo $LIBEWF_FNAME | sed s/-experimental// | sed s/.tar.gz//)
echo
echo "Now installing libewf into $LIBEWF_DIR"
wget -nv $LIBEWF_URL || (echo could not download $LIBEWF_URL. Stop; exit 1)
tar xfz $LIBEWF_FNAME || (echo could not untar $LIBEWF_FNAME. Stop; exit 1)
(cd $LIBEWF_DIR  \
     && ./configure --quiet --enable-silent-rules --prefix=/usr/local \
     && make \
     && sudo make install) || (echo could not build libewf. Stop; exit 1)
echo Cleaning up $LIBEWF_FNAME and $LIBEWF_DIR
/bin/rm -rf $LIBEWF_FNAME $LIBEWF_DIR || (echo could not clean up. Stop; exit 1)

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
