LATEXMK ?= latexmk

.PHONY: all clean

all: BEProgrammersManual.pdf

BEProgrammersManual.pdf: BEProgrammersManual.tex references.bib \
	extractinformationreprocess.pdf histogramprocessing.pdf \
	margindepiction.pdf scannerloop.pdf scannersextractfeatures.pdf \
	system_diagram.pdf writetoemail.pdf
	$(LATEXMK) -pdf -halt-on-error -file-line-error $<

clean:
	$(LATEXMK) -C BEProgrammersManual
