set term png
set output "graph.png"
set logscale x
plot \
	"results.csv" u 1:($2/$1) w lines title 'Cache misses per element (B-tree)', \
	"results.csv" u 1:($5/$1) w lines title 'Cache misses per element (COB-tree)', \
	"results.csv" u 1:($4/$1) axes x1y2 w lines title 'Time per element (B-tree)', \
	"results.csv" u 1:($7/$1) axes x1y2 w lines title 'Time per element (COB-tree)'

set output "graph2.png"
plot \
	"results.csv" u 1:($4/$1) axes x1y2 w lines title 'Time per element (B-tree)', \
	"results.csv" u 1:($7/$1) axes x1y2 w lines title 'Time per element (COB-tree)', \
	x title 'N'

set output "graph3.png"
plot \
	"results.csv" u 1:($8/$1) w lines title 'Total reorganized elements / N', \
	log(x)**2 axes x1y2 title 'log(N)^2'

set output "graph4.png"
plot \
	"results.csv" u 1:($8/$1) w lines title 'Total reorganized elements / N', \
	x axes x1y2 title 'N'

#	log(x)**2 title 'log(x)^2'

#	"results.csv" u 1:($3/$1) w lines title 'Cache references per element (B-tree)', \
#	"results.csv" u 1:($6/$1) w lines title 'Cache references per element (COB-tree)'
