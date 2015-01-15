set term png
set output "graph1.png"
# set logscale x
set key outside below
set xlabel 'Van Emde Boas tree height'
set y2label 'ns'
set y2tics
plot \
	"results.csv" u 1:($3/$2) w lines title 'Cache misses per lookup', \
	"results.csv" u 1:($4/$2) w lines title 'Cache references per lookup', \
	"results.csv" u 1:($5/$2) axes x1y2 w lines title 'Time per lookup' linewidth 2
