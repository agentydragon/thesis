#!/bin/bash
# Exports currently generated images into //text/img for commiting.
rm ../../../text/img/performance/*.png
cp output/export/* ../../../text/img/performance
