all: thesis.pdf

PDFLATEX_FLAGS=-jobname thesis

# LaTeX must be run twice to get references right

thesis.bbl: thesis.tex $(wildcard *.tex) bibliography.bib
	rm -f thesis.bbl
	pdflatex $(PDFLATEX_FLAGS) thesis.tex
	bibtex thesis

thesis.pdf: thesis.tex $(wildcard *.tex) thesis.bbl
	pdflatex $(PDFLATEX_FLAGS) $<
	pdflatex $(PDFLATEXFLAGS) $<

clean:
	rm -f *.{log,dvi,aux,toc,lof,out} thesis.pdf
