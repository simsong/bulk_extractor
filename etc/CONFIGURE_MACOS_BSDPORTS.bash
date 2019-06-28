#!/bin/bash
cat <<EOF
*******************************************************************
        Configuring MacOS with MacPorts
*******************************************************************

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

PKGS+="openssl wget bison libtool autoconf automake flex bison"

if [ ! -x /opt/local/bin/port ]; then
    echo This script assumes that MacPorts is installed.
    exit 1
fi

echo Will now try to install 

# I use emacs. Installing it may install requiremnts
#sudo port selfupdate
#sudo port upgrade outdated
#sudo port upgrade installed
sudo port -Nc install $PKGS || (echo port install failed; exit 1)
exit 0

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
