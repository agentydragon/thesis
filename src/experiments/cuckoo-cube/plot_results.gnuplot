set term png
set output "results.png"
set logscale x
plot "output.csv" u ($1**3):2 w lines title 'Number of full rehashes', \
	(x**(0.3333)) * 0.1 w lines
