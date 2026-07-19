#!/bin/sh

set -eu

mkdir -p build-aux

# Make sure generated distribution and RAR manifests are current.
python3 etc/makefile_builder.py

# Regenerate a consistent Autotools set with the installed tool versions.
autoreconf --force --install

# We were very excited about AddressSanitizer.
# This is how to enable it...
ADVERTISE=no
if [ $ADVERTISE = 'yes' ]; then
  if [ `uname -s` = 'Darwin' ]; then
    echo To enable AddressSanitizer on Mac, you must install gcc-4.8 with macports, then:
    echo CC=gcc-mp-4.8 CXX=g++-mp-4.8  sh configure --enable-address-sanitizer
  else
    echo To enable AddressSanitizer, install libasan and configure with:
    echo sh configure --enable-address-sanitizer
  fi
fi

# bootstrap is complete
echo
echo The bootstrap.sh is complete.  Be sure to run ./configure.
echo
