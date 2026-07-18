#!/bin/sh

set -eu

mkdir -p build-aux

# Make sure the current submodule layout was checked out recursively. Update
# once if anything is missing; do not probe removed historical submodules.
missing_submodule=no
for sub in dfxml_schema src/be20_api src/be20_api/dfxml_cpp src/be20_api/utfcpp
do
  if [ ! -e "$sub/.git" ]; then
    echo "submodule $sub is not present."
    missing_submodule=yes
  fi
done

if [ "$missing_submodule" = yes ]; then
  echo 'Attempting recursive submodule initialization...'
  git submodule update --init --recursive
fi

# Makesure files are in src/Makefile.auto_defs
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
