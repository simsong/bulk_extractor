#!/bin/bash
if [ -r /usr/local/bin/brew ]; then
    WHICH=/usr/local/bin/brew
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
PKGS+="wget libtool autoconf automake"

$WHICH install $PKGS || (echo installation install failed; exit 1)
exit 0
