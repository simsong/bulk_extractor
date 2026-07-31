# Documentation examples use headers from the integrated be20 component.
AM_CPPFLAGS = -I${top_srcdir}/src/be20_api
AUTOMAKE_OPTIONS = subdir-objects

EXTRA_DIST = carving.md logging.md scanner_api.md scanner_template.cpp \
    programmer_manual/BEProgrammersManual.tex \
    programmer_manual/Makefile.am \
    programmer_manual/extractinformationreprocess.pdf \
    programmer_manual/histogramprocessing.pdf \
    programmer_manual/margindepiction.pdf \
    programmer_manual/scannerloop.pdf \
    programmer_manual/scannersextractfeatures.pdf \
    programmer_manual/system_diagram.pdf \
    programmer_manual/writetoemail.pdf

.PHONY: pdfs
pdfs:
	$(MAKE) -C latex_manuals -f Makefile.am pdfs
	$(MAKE) -C programmer_manual -f Makefile.am pdfs


DOXYGEN_FILES = doxygen/Doxyfile doxygen/Makefile doxygen/programmer_manual.doxygen doxygen/README \
	doxygen/images/BEViewer_blank.eps doxygen/images/BEViewer_blank.png
