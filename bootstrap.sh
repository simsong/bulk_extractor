#!/bin/sh

mkdir -p build-aux

# Make sure it was checked out with git clone --recursive ...
# If not, try to fix it. dfxml_schema is there twice to verify that the checkout worked

for sub in dfxml_schema dfxml_schema src/be20_api src/be20_api/dfxml_cpp src/be20_api/utfcpp src/be20_api/utfcpp/extern/ftest
do
  if [ ! -r $sub/.git ] ;  then
      echo submodule $sub is not present.
      echo 'When you did your original "git pull", you likely did not include the --recursive flag.'
      echo 'Attempting to fix...'
      git submodule init
      git submodule update
      git submodule foreach --recursive 'git submodule init;git submodule update'
  fi
done

# Makesure files are in src/Makefile.auto_defs
python3 etc/makefile_builder.py

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
