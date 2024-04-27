#!/bin/bash

MYDIR=$(dirname $(readlink -f $0))

source $MYDIR/paths.bash
if [ -r /usr/local/bin/brew ]; then
    WHICH=/usr/local/bin/brew
elif [ -r /opt/homebrew/bin/brew ]; then
    WHICH=/opt/homebrew/bin/brew
elif [ -r /opt/local/bin/port ]; then
    WHICH="sudo /opt/local/bin/port -Nc"
else
    echo Cannot find brew or macports executable.
    exit 1
fi

cat <<EOF
*******************************************************************
Configuring MacOS with $WHICH
*******************************************************************

press any key to continue...
EOF
read

# Note: openssl no longer required
# Apple's provided flex is 2.6.4, which is the same that is provided by brew
PKGS+="wget libtool autoconf automake libtool libxml2 libewf json-c re2 abseil pkg-config"

$WHICH install $PKGS || (echo installation install failed; exit 1)

echo ================
echo Updating .bashrc
echo 'PATH="/opt/homebrew/opt/libtool/libexec/gnubin:$PATH"' >> $HOME/.bashrc
echo 'export PATH="/opt/homebrew/opt/libxml2/bin:$PATH"' >> $HOME/.bashrc
echo 'export LDFLAGS="-L/opt/homebrew/lib -L/opt/homebrew/opt/libxml2/lib $LDFLAGS" ' >> $HOME/.bashrc
echo 'export CPPFLAGS="-I/opt/homebrew/include -I/opt/homebrew/opt/libxml2/include $CPPFLAGS" ' >> $HOME/.bashrc
echo 'export PKG_CONFIG_PATH="/opt/homebrew/opt/libxml2/lib/pkgconfig:$PKG_CONFIG_PATH"' >> $HOME/.bashrc
echo ================
source $HOME/.bashrc
printenv
exit 0
