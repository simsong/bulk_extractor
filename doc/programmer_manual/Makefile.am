LATEXMK ?= latexmk
PDFS = BEProgrammersManual.pdf

.PHONY: all pdfs clean

all pdfs: $(PDFS)

%.pdf: %.tex
	$(LATEXMK) -pdf -halt-on-error -file-line-error $<

clean:
	$(LATEXMK) -C $(basename $(PDFS))
