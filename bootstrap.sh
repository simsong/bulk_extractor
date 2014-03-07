#!/bin/sh
# Hopefully you checked out with git clone --recursive git@github.com:simsong/bulk_extractor.git

for sub in be13_api dfxml sceadan
do
  if [ ! -r src/$sub/.git ] ;
  then
    echo bringing in submodules
    echo next time check out with git clone --recursive
    git submodule init
    git submodule update
  fi
done

# have automake do an initial population iff necessary
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
echo be sure to run ./configure
