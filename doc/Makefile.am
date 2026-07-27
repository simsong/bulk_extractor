# Documentation examples use headers from the integrated be20 component.
AM_CPPFLAGS = -I${top_srcdir}/src/be20_api
AUTOMAKE_OPTIONS = subdir-objects

EXTRA_DIST = scanner_api.md scanner_template.cpp

.PHONY: pdfs
pdfs:
	$(MAKE) -C latex_manuals -f Makefile.am pdfs
	$(MAKE) -C programmer_manual -f Makefile.am pdfs


DOXYGEN_FILES = doxygen/Doxyfile doxygen/Makefile doxygen/programmer_manual.doxygen doxygen/README \
	doxygen/images/BEViewer_blank.eps doxygen/images/BEViewer_blank.png
