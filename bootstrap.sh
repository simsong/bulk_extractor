#!/bin/sh

set -eu

mkdir -p build-aux

missing_submodule=no
for required in \
  dfxml_schema/dfxml.xsd \
  src/be20_api/dfxml_cpp/src/dfxml_writer.h \
  src/be20_api/utfcpp/source/utf8.h
do
  if [ ! -f "$required" ]; then
    echo "Missing dependency submodule file: $required" >&2
    missing_submodule=yes
  fi
done
if [ "$missing_submodule" = yes ]; then
  echo "Run: git submodule update --init" >&2
  exit 1
fi

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
