#!/bin/sh
# Make sure it was checked out with git clone --recursive ...
# If not, update the submodules

for sub in be13_api dfxml 
do
  if [ ! -r src/$sub/.git ] ;  then
    echo bringing in submodules
    echo next time check out with git clone --recursive
    git submodule init
    git submodule update
  fi
done

# have automake do an initial population if necessary
if [ ! -e config.guess -o ! -e config.sub -o ! -e install-sh -o ! -e missing ]; then
    autoheader -f
    touch NEWS README AUTHORS ChangeLog
    touch stamp-h
    aclocal -I m4
    autoconf -f
    #libtoolize || glibtoolize
    automake --add-missing --copy
else
    autoreconf -f
fi

if [ `uname -s` = 'Darwin' ]; then
  echo To enable AddressSanitizer on Mac, you must install gcc-4.8 with macports, then:
  echo CC=gcc-mp-4.8 CXX=g++-mp-4.8  sh configure --enable-address-sanitizer
else
  echo To enable AddressSanitizer, install libasan and configure with:
  echo sh configure --enable-address-sanitizer
fi

# bootstrap is complete
echo
echo The bootstrap.sh is complete.  Be sure to run ./configure.
echo
