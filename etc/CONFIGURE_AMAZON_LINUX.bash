#!/bin/bash
cat <<EOF
*******************************************************************
        Configuring Amazon Linux for compiling bulk_extractor
*******************************************************************

This script will configure a fresh Amazon Linux system to compile 
bulk_extractor.  Please perform the following steps:

1. Start a VM
2. sudo yum -y update
3. sudo yum -y install git
4. git clone https://github.com/simsong/bulk_extractor.git
5. cd bulk_extractor
6. sudo sh misc/CONFIGURE_AMAZON_LINUX.bash
7. make && sudo make install

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

MPKGS="autoconf automake flex gcc gcc-c++ git libtool "
MPKGS+="md5deep wget bison zlib-devel "
MPKGS+="libewf libewf-devel java-1.8.0-openjdk-devel "
MPKGS+="libxml2-devel libxml2-static openssl-devel "
MPKGS+="expat-devel "

if [ ! -r /etc/os-release ]; then
  echo This requires Amazon Linux
  exit 1
fi

. /etc/os-release
if [ $ID != 'amzn' ]; then
  echo This requires Amazon Linux
  exit 1
fi

echo Will now try to install 

sudo yum install -y $MPKGS --skip-broken
if [ $? != 0 ]; then
  echo "Could not install some of the packages. Will not proceed."
  exit 1
fi

echo
echo "Now performing a yum update to update system packages"
sudo yum -y update


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
echo 'You are now ready to compile bulk_extractor for Linux.'
echo 'To compile, type make'
echo 'To make a distribution, type make release'