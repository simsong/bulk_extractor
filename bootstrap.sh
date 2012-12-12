#!/bin/sh
# Hopefully you checked out with git clone --recursive git@github.com:simsong/bulk_extractor.git

if [ ! -d src/be13_api/.git ] ;
then
  echo bringing in submodules
  echo next time check out with git clone --recursive
  git submodule init
  git submodule update
fi

autoheader -f
touch NEWS README AUTHORS ChangeLog
touch stamp-h
aclocal -I m4
autoconf -f
#libtoolize || glibtoolize
automake --add-missing --copy
