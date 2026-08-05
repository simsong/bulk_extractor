LATEXMK ?= latexmk
PDFS = BECurrentGuide.pdf
USER_MANUAL_ASSETS = \
	archPics/howitworks.pdf \
	archPics/margindepiction.pdf \
	installPics/installer1.png \
	installPics/installer_download.pdf \
	installPics/windowsWarning2.pdf \
	otherPics/LightgrepCheatSheet.pdf \
	otherPics/U+0428.pdf \
	otherPics/U+0428.txt \
	otherPics/U+062D.pdf \
	otherPics/U+062D.txt \
	otherPics/U+2234.pdf \
	otherPics/U+2234.txt \
	otherPics/U+611B.pdf \
	otherPics/U+611B.txt \
	otherPics/WithWithoutStopList.jpg \
	otherPics/forensicPath.jpg \
	viewerPics/emailFeatureFile.png \
	viewerPics/emailHistogramView.png \
	viewerPics/listOfOutput.png \
	viewerPics/reportIsReady.png \
	viewerPics/runBulk.png \
	viewerPics/runBulkSnippet.png \
	viewerPics/runCompleteStatus.png \
	viewerPics/selectOutputDirectory.png \
	viewerPics/startup.png
USER_MANUAL_DEPS = BECurrentGuide.tex BEWorkedExamples.tex references.bib \
	$(USER_MANUAL_ASSETS) carvedJPEG.jpg charlieRunOutput.png

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
