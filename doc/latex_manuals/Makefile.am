LATEXMK ?= latexmk
PDFS = BECurrentGuide.pdf
USER_MANUAL_DEPS = BEWorkedExamples.tex references.bib \
	$(wildcard archPics/*.pdf installPics/* otherPics/* viewerPics/*) \
	carvedJPEG.jpg charlieRunOutput.png

EXTRA_DIST = $(USER_MANUAL_DEPS) index.html assets/bulk-extractor-icon.svg

.PHONY: all doc pdfs clean

all:
	@echo "Run make pdfs to build the LaTeX manuals."
doc pdfs: $(PDFS)

BECurrentGuide.pdf: $(USER_MANUAL_DEPS)

%.pdf: %.tex
	$(LATEXMK) -pdf -halt-on-error -file-line-error $<

clean:
	$(LATEXMK) -C $(basename $(PDFS))
