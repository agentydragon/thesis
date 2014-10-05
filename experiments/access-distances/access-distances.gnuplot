plot \
     "findings.tsv" u 1:2 w lines title 'Cost of access to X bytes / access to 1 byte', \
     "findings.tsv" u 1:3 w lines title 'Cost of access to 2 bytes X apart / access to 1 byte'
