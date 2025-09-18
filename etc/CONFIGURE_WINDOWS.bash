#!/bin/bash

$LIBEWF_URL =

pacman -Syu --noconfirm

PKGS+="base-devel automake autoconf pkgconf
         mingw-w64-ucrt-x86_64-gcc
         mingw-w64-ucrt-x86_64-make
         mingw-w64-ucrt-x86_64-re2
         mingw-w64-ucrt-x86_64-abseil-cpp
         mingw-w64-ucrt-x86_64-sqlite3
         mingw-w64-ucrt-x86_64-openssl
         mingw-w64-ucrt-x86_64-expat"

pacman -S --needed --noconfirm $PKGS || (echo msys package install failed; exit 1)

source $SCRIPT_DIR/paths.bash

# Install libewf
$WGET $LIBEWF_URL || (echo could not download $LIBEWF_URL; exit 1)
tar xfz libewf*gz   && (cd libewf*/   && $CONFIGURE && $MAKE >/dev/null && sudo make install)
ls -l /etc/ld.so.conf.d/
sudo ldconfig
ewfinfo -h >/dev/null || (echo could not install libewf; exit 1)

exit 0
