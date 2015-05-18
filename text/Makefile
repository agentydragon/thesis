all: thesis.pdf

thesis.pdf: thesis.tex $(wildcard *.tex)
	latexmk -pdf thesis.tex

clean:
	rm -f *.{log,dvi,aux,toc,lof,out} *.bbl *.blg *.fdb_latexmk *.fls *.lot thesis.pdf
