/bin/rm -rf aclocal.m4
autoheader -f
touch NEWS README AUTHORS ChangeLog
touch stamp-h
aclocal -I m4
autoconf -f
#libtoolize || glibtoolize
automake --add-missing --copy
