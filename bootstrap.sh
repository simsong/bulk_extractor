#!/bin/sh
# Hopefully you checked out with git clone --recursive git@github.com:simsong/bulk_extractor.git

for sub in be13_api dfxml 
do
  if [ ! -r src/$sub/.git ] ;
  then
    echo bringing in submodules
    echo next time check out with git clone --recursive
    git submodule init
    git submodule update
  fi
done

# git submodule update
autoheader -f
touch NEWS README AUTHORS ChangeLog
touch stamp-h
aclocal -I m4
autoconf -f
#libtoolize || glibtoolize
automake --add-missing --copy
echo be sure to run ./configure
