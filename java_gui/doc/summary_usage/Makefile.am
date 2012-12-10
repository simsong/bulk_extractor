images = images/BEViewer_example.pdf images/BEViewer_blank_annotated.pdf

EXTRA_DIST=summary-usage.tex $(images) images/Makefile

CLEANFILES = summary-usage.aux summary-usage.log summary-usage.out summary-usage.pdf

all: summary-usage.pdf

# run pdflatex twice to get Figures right
summary-usage.pdf: summary-usage.tex $(images)
	pdflatex summary-usage
	pdflatex summary-usage

