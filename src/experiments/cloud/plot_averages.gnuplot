set terminal png size 800,600
set xlabel 'Longitude'
set ylabel 'Latitude'

unset key

tfm(lat) = (lat > 18000) ? (lat - 36000) : lat;

set output "wind-speed.png"
set title 'Wind speed (m/s)'
plot "averages.csv" u (tfm($2)/100):($1/100):($4/10) w points pointsize 0.6 pointtype 7 palette, \
	"world_50m.txt" u 1:2 w lines linetype rgb "#000000"

set output "stations.png"
set notitle
plot "world_50m.txt" u 1:2 w lines linetype rgb "#000000", \
	"positions.csv" u (tfm($2)/100):($1/100) w points pointsize 0.4

set output "pressure.png"
set title 'Sea-level atmospheric pressure (mbar)'
plot "averages.csv" u (tfm($2)/100):($1/100):($3/10) w points pointsize 0.6 pointtype 7 palette, \
	"world_50m.txt" u 1:2 w lines linetype rgb "#000000"
