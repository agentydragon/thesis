set term png
set output "graph1.png"
set logscale x
set key outside below
plot \
	"results.csv" u 1:($2/$1) w lines title 'Cache misses per element (B-tree)', \
	"results.csv" u 1:($5/$1) w lines title 'Cache misses per element (COB-tree)', \
	"results.csv" u 1:($4/$1) axes x1y2 w lines title 'Time per element (B-tree)' linewidth 2, \
	"results.csv" u 1:($7/$1) axes x1y2 w lines title 'Time per element (COB-tree)' linewidth 2

set output "graph1-findonly.png"
plot \
	"results.csv" u 1:($8/$1) w lines title 'Cache misses per element (B-tree)', \
	"results.csv" u 1:($11/$1) w lines title 'Cache misses per element (COB-tree)', \
	"results.csv" u 1:($10/$1) axes x1y2 w lines title 'Time per element (B-tree)' linewidth 2, \
	"results.csv" u 1:($13/$1) axes x1y2 w lines title 'Time per element (COB-tree)' linewidth 2

set output "graph2.png"
plot \
	"results.csv" u 1:($4/$1) axes x1y2 w lines title 'Time per element (B-tree)', \
	"results.csv" u 1:($7/$1) axes x1y2 w lines title 'Time per element (COB-tree)'

set output "graph2-findonly.png"
plot \
	"results.csv" u 1:($10/$1) axes x1y2 w lines title 'Time per element (B-tree)', \
	"results.csv" u 1:($13/$1) axes x1y2 w lines title 'Time per element (COB-tree)'

set output "graph3.png"
plot \
	"results.csv" u 1:($14/$1) w lines title 'Total reorganized elements / N'
