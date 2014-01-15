PDFS=BEUsersManual.pdf BEWorkedExamplesStandalone.pdf BETheoryOfOperation.pdf

all:	
	echo type \"make doc\" to create documentation

doc: $(PDFS)

clean:
	/bin/rm -f BEUsersManual.pdf BEWorkedExamplesStandalone.pdf *.log *.aux *.bbl *.bst *.blg *~ *.toc

pub: $(PDFS)
	scp $(PDFS) ncr.nps.edu:/var/www/digitalcorpora/downloads/bulk_extractor/

SUFFIXES=.tex .pdf
.tex.pdf:
	pdflatex $*
	if [-e $*.bib] ; then bibtex $* ; pdflatex $* ; fi
	pdflatex $*
