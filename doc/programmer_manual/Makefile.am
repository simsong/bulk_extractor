LATEXMK ?= latexmk
PDFS = BEProgrammersManual.pdf
PROGRAMMER_MANUAL_DEPS = references.bib extractinformationreprocess.pdf \
	histogramprocessing.pdf margindepiction.pdf scannerloop.pdf \
	scannersextractfeatures.pdf system_diagram.pdf writetoemail.pdf

.PHONY: all pdfs clean

all pdfs: $(PDFS)

BEProgrammersManual.pdf: $(PROGRAMMER_MANUAL_DEPS)

%.pdf: %.tex
	$(LATEXMK) -pdf -halt-on-error -file-line-error $<

clean:
	$(LATEXMK) -C $(basename $(PDFS))
