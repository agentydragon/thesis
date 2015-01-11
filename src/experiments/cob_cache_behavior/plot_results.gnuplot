set term png
set output "graph.png"
set logscale x
plot \
	"results.csv" u 1:($2/$1) w lines title 'Cache misses per element (B-tree)', \
	"results.csv" u 1:($5/$1) w lines title 'Cache misses per element (COB-tree)', \
	"results.csv" u 1:($4/$1) axes x1y2 w lines title 'Time per element (B-tree)', \
	"results.csv" u 1:($7/$1) axes x1y2 w lines title 'Time per element (COB-tree)'

#	"results.csv" u 1:($3/$1) w lines title 'Cache references per element (B-tree)', \
#	"results.csv" u 1:($6/$1) w lines title 'Cache references per element (COB-tree)'
