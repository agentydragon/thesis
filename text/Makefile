all: thesis.pdf

thesis.pdf: thesis.tex $(wildcard *.tex)
	latexmk -pdf thesis.tex

clean:
	rm -f *.{log,dvi,aux,toc,lof,out} thesis.pdf
