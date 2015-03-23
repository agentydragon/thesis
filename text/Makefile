all: thesis.pdf

# LaTeX must be run twice to get references right
thesis.pdf: thesis.tex $(wildcard *.tex)
	pdflatex $< output
	pdflatex $< output

clean:
	rm -f *.{log,dvi,aux,toc,lof,out} thesis.pdf
