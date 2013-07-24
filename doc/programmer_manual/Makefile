PARTS=BEProgrammersManual.tex title.tex structure.tex 

BEProgrammersManual.pdf: $(PARTS) Makefile references.bib
	pdflatex BEProgrammersManual
	bibtex BEProgrammersManual
	pdflatex BEProgrammersManual
	pdflatex BEProgrammersManual

clean:
	rm -f *.{aux,bbl,blg,log,out,toc} *~
	rm -f BEProgrammersManual.pdf
