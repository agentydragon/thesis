all: output/thesis.pdf

# Vyroba PDF primo z DVI by byla prijemnejsi, ale vetsina verzi dvipdfm nici obrazky
# output/thesis.pdf: output/thesis.dvi
#	dvipdfm -o $@ -p a4 -r 600 $<

output/thesis.pdf: output/thesis.ps
	ps2pdf $< $@

output/thesis.ps: output/thesis.dvi
	dvips -o $@ -D600 -t a4 $<

# LaTeX je potreba spustit dvakrat, aby spravne spocital odkazy
output/thesis.dvi: thesis.tex $(wildcard *.tex)
	cslatex -output-directory=output $<
	cslatex -output-directory=output $<

clean:
	rm -f output/*.{log,dvi,aux,toc,lof,out} output/thesis.ps output/thesis.pdf
