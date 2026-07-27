LATEXMK ?= latexmk

.PHONY: all clean

all: BEProgrammersManual.pdf

BEProgrammersManual.pdf: BEProgrammersManual.tex
	$(LATEXMK) -pdf -halt-on-error -file-line-error $<

clean:
	$(LATEXMK) -C BEProgrammersManual
