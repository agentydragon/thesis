#!/bin/bash

cd data

for YEAR in 97 98 99 00 01 02 03 04 05 06 07 08 09; do
	for MONTH in JAN FEB MAR APR MAY JUN JUL AUG SEP OCT NOV DEC; do
		FILE="${MONTH}${YEAR}L"
		if [ ! -f $FILE ]; then
			wget http://cdiac.ornl.gov/ftp/ndp026c/land_199701_200912/${FILE}.Z
			uncompress ${FILE}.Z
		fi
	done
done
