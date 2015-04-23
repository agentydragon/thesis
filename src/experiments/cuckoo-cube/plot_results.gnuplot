# set term pdf
# set output "results.pdf"

set format '$$%g$$';
set terminal epslatex standalone color;
set output 'results.tex'
set xlabel 'Number of inserted elements'

set logscale x
plot "output.csv" u ($1**3):2 w lines title 'Number of full rehashes', \
	(x**(0.3333)) * 0.2 + 10 w lines title '$\mathcal{O}(N^{1/3})$ (for reference)'
