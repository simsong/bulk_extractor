#!/bin/sh
# Make sure it was checked out with git clone --recursive ...
# If not, update the submodules

mkdir -p build-aux

for sub in src/be13_api dfxml_schema
do
  if [ ! -r $sub/.git ] ;  then
    echo submodule $sub is not present.
    exit 1
  fi
done

# have automake do an initial population if necessary
autoheader -f
touch NEWS README AUTHORS ChangeLog
touch stamp-h
aclocal -I m4
autoconf -f
automake --add-missing --copy

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
