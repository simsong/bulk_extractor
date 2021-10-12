#!/bin/sh

# Originally from https://gist.github.com/GraemeConradie/49d2f5962fa72952bc6c64ac093db2d5
# Install gnu autotools for running under github actions

##
# Install autoconf, automake and libtool smoothly on Mac OS X.
# Newer versions of these libraries are available and may work better on OS X
##

AUTOCONF=https://ftpmirror.gnu.org/autoconf/autoconf-2.69.tar.gz
AUTOMAKE=https://ftpmirror.gnu.org/automake/automake-1.16.3.tar.gz
LIBTOOL=https://ftpmirror.gnu.org/libtool/libtool-2.4.6.tar.gz

export BUILD_DIR=~/devtools # or wherever you'd like to build
mkdir -p $BUILD_DIR

function build () {
    cd $BUILD_DIR
    echo Downloading and installing $1
    curl -s -OL $1 || (echo could not download $1; exit 1)
    tar xzf $(basename $1) || (echo could not untar $1; exit 1)
    (cd $(basename $1 | sed s/.tar.gz//) \
         && ./configure --quiet --prefix=/usr/local  \
         && make \
         && sudo make install) || (echo installing $1 failed; exit 1)
}

if grep ID=fedora /etc/os-release >/dev/null ; then
    sudo yum install perl-File-Compare perl-File-Copy
fi

build $AUTOCONF || exit 1
build $AUTOMAKE || exit 1
build $LIBTOOL || exit 1

# Make sure that /usr/local/lib is in ldconfig
sudo /bin/rm -f /tmp/local.conf
echo /usr/local/lib > /tmp/local.conf
sudo mv /tmp/local.conf /etc/ld.so.conf.d/local.conf
sudo ldconfig


echo "Installation complete."
