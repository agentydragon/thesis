set term png
set grid
set output "graph1.png"
set logscale x
set key outside below
set y2tics
set ylabel 'cache misses'
set y2label 'ns'
set xrange [100:]
plot \
	"results.tsv" u 1:($2/$1) w lines title 'Cache misses per element (B-tree)', \
	"" u 1:($5/$1) w lines title 'Cache misses per element (COB-tree)', \
	"" u 1:($8/$1) w lines title 'Cache misses per element (splay)', \
	"" u 1:($4/$1) axes x1y2 w lines title 'Time per element (B-tree)' linewidth 2, \
	"" u 1:($7/$1) axes x1y2 w lines title 'Time per element (COB-tree)' linewidth 2, \
	"" u 1:($10/$1) axes x1y2 w lines title 'Time per element (splay)' linewidth 2

set output "graph1-findonly.png"
plot \
	"results.tsv" u 1:($11/$1) w lines title 'Cache misses per element (B-tree)', \
	"" u 1:($14/$1) w lines title 'Cache misses per element (COB-tree)', \
	"" u 1:($17/$1) w lines title 'Cache misses per element (splay)', \
	"" u 1:($13/$1) axes x1y2 w lines title 'Time per element (B-tree)' linewidth 2, \
	"" u 1:($16/$1) axes x1y2 w lines title 'Time per element (COB-tree)' linewidth 2, \
	"" u 1:($19/$1) axes x1y2 w lines title 'Time per element (splay)' linewidth 2

set output "graph1-insertonly.png"
plot \
	"results.tsv" u 1:(($2-$11)/$1) w lines title 'Cache misses per element (B-tree)', \
	"" u 1:(($5-$14)/$1) w lines title 'Cache misses per element (COB-tree)', \
	"" u 1:(($8-$17)/$1) w lines title 'Cache misses per element (splay)', \
	"" u 1:(($4-$13)/$1) axes x1y2 w lines title 'Time per element (B-tree)' linewidth 2, \
	"" u 1:(($7-$16)/$1) axes x1y2 w lines title 'Time per element (COB-tree)' linewidth 2, \
	"" u 1:(($10-$19)/$1) axes x1y2 w lines title 'Time per element (splay)' linewidth 2

set output "graph2.png"
plot \
	"results.tsv" u 1:($4/$1) axes x1y2 w lines title 'Time per element (B-tree)', \
	"" u 1:($7/$1) axes x1y2 w lines title 'Time per element (COB-tree)', \
	"" u 1:($10/$1) axes x1y2 w lines title 'Time per element (splay)'

set output "graph2-findonly.png"
plot \
	"results.tsv" u 1:($13/$1) axes x1y2 w lines title 'Time per element (B-tree)', \
	"" u 1:($16/$1) axes x1y2 w lines title 'Time per element (COB-tree)', \
	"" u 1:($19/$1) axes x1y2 w lines title 'Time per element (splay)'

set output "working-set-1k-findonly.png"
plot \
	"results.tsv" u 1:($22/$1) axes x1y2 w lines title 'Time per element (B-tree)', \
	"" u 1:($25/$1) axes x1y2 w lines title 'Time per element (COB-tree)', \
	"" u 1:($28/$1) axes x1y2 w lines title 'Time per element (splay)'

set output "working-set-100k-findonly.png"
plot \
	"results.tsv" u 1:($31/$1) axes x1y2 w lines title 'Time per element (B-tree)', \
	"" u 1:($34/$1) axes x1y2 w lines title 'Time per element (COB-tree)', \
	"" u 1:($37/$1) axes x1y2 w lines title 'Time per element (splay)'

set output "graph3.png"
set noylabel
set noy2label
plot \
	"results.tsv" u 1:($38/$1) w lines title 'Total reorganized elements / N'
