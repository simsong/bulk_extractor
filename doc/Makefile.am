# So that relative imports from be13_api work
AM_CPPFLAGS = -I${top_srcdir}/src/be13_api
AUTOMAKE_OPTIONS = subdir-objects


DOXYGEN_FILES = doxygen/Doxyfile doxygen/Makefile doxygen/programmer_manual.doxygen doxygen/README \
	doxygen/images/BEViewer_blank.eps doxygen/images/BEViewer_blank.png

EXTRA_DIST = \
	*.txt \
	*.pdf \
	*.md \
	latex_manuals/viewerPics/*.png \
	announce/*.txt \
	announce/*.md \
	bulk_extractor.html \
	Diagnostic_Notes/be_crash_diagnostics.md \
	Diagnostic_Notes/bulk_extractor.md
	Diagnostic_Notes/crash_diagnosis1.txt \
	Diagnostic_Notes/crash_diagnosis2.txt\
	Diagnostic_Notes/debugging_performance_problems.html \
	Diagnostic_Notes/open_source_help \
	Diagnostic_Notes/using_gdb \
	performance.txt \
	$(DOXYGEN_FILES)
