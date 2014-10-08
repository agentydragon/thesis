text.aux: text.tex
	pdflatex text.tex

text.bbl: bibliography.bib text.aux
	bibtex text.aux

text.pdf: text.tex text.bbl
	pdflatex text.tex
