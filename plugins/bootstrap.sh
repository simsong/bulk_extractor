#!/bin/sh

# build the makefiles
autoheader -f
aclocal -I m4
autoconf -f
automake --add-missing --copy
echo be sure to run ./configure
