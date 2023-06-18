#!/bin/bash
source paths.bash
OS_NAME=gentoo
OS_VERSION=2.13
if [ ! -r /etc/os-release ]; then
  echo This requires /etc/os-release
  exit 1
fi
. /etc/os-release
if [ "$ID" != $OS_NAME ]; then
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
EOF

if [ "$1" != '-nowait' ]; then
    echo press any key to continue...
    read
fi

# cd to the directory where the script is
cd "$( dirname "${BASH_SOURCE[0]}" )"

make_libewf

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
