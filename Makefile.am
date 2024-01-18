# bulk_extractor Makefile.am
# This file is compiled with automake to create Makefile.in.
# Makefile.in is transformed by "configure" to create Makefile
#
# (C) 2020 Simson L. Garfinkel
# https://www.gnu.org/licenses/lgpl-3.0.en.html

# We need to list *all* of the subdirs here that have Makefiles in them that are executed by
# doing a chdir into the directory and typing 'make'. So we do not need src/be20_api
SUBDIRS = doc doc/latex_manuals man src src/tests python specfiles tests

# Include things explicitly
# Note - autoconf seems to automatically include the m4 rules that are used by configure.ac - no need to list them heren
#
# Don't include bootstrap. People run it, and they shouldn't
# It's only for people who check out the git repo
#
# note - we are allowed to use wildcards below! Who knew.
# and using $wildcard prevents it from running on mac, which doesn't use gnu make

EXTRA_DIST = \
	$(SRC_WIN_DIST) \
	$(srcdir)/*.md \
	$(srcdir)/.gitignore \
	$(srcdir)/bootstrap.sh \
	$(srcdir)/etc/*.bash \
	$(srcdir)/licenses/LICENSE.* \
	$(srcdir)/*.txt \
	$(srcdir)/src/be20_api/dfxml_cpp/src/Makefile.defs


# ACLOCAL_AMFLAGS = ${ACLOCAL_FLAGS} -I m4

distclean2:
	@echo Deleting:
	@find . \
        \( -name '*.orig' -o -name '*.rej' -o -name '*~' \
        -o -name '*.bak' -o -name '#*#' -o -name '.*.orig' \
        -o -name '.*.rej' -o -size 0 \
        -o -name '*%' -o -name '.*.cmd' -o -name 'core' \) \
        -type f -print | tee /dev/tty | xargs rm -f

.PHONY: exels
exels:
	/bin/ls -l */*exe

.PHONY: execlean
execlean:
	/bin/rm -f */*exe

.PHONY: win64
win64: win64/bulk_extractor64.exe

win64/bulk_extractor64.exe:
	@echo '****************************************************'
	@echo '*** making win64 with mingw32-configure and make ***'
	@echo '****************************************************'
	if [ -r Makefile ]; then $(MAKE) distclean ; fi
	if [ ! -d win64  ]; then mkdir win64 ; fi
	if [ ! -r win64/Makefile ]; then (cd win64;mingw64-configure) ; fi
	(cd win64;$(MAKE))
	(cp win64/src/bulk_extractor.exe win64/bulk_extractor64.exe)
	ls -l win64/bulk_extractor64.exe

winlibs:
	@echo '*****************************'
	@echo '*** making win64 library  ***'
	@echo '*****************************'
	if [ -r Makefile ]; then $(MAKE) distclean ; fi
	if [ ! -d win64  ]; then mkdir win64 ; fi
	if [ ! -r win64/Makefile ]; then (cd win64;mingw64-configure --disable-libewf --disable-afflib) ; fi
	(cd win64/src;$(MAKE) lib;mv libbulkextractor.so ../../libbulkextractor64.DLL)
	ls -l libbulk*.DLL

.PHONY: lib
lib:
	(cd src; $(MAKE) lib)

.PHONY: libinstall
libinstall:
	install -D src/libbulkextractor.so $(prefix)/lib/libbulkextractor.so
	install -D python/module/bulkextractor.py $(prefix)/lib/python2.7/dist-packages/bulkextractor.py
	if [ -r /sbin/ldconfig ]; /sbin/ldconfig -v ; fi
