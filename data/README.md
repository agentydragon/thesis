## README for /data/

This directory contains the hash table operation recordings from a Mozilla
Firefox session. The recording may be replayed using ../src/bin/experiments/vcr
or ../src/experiments/vcr/make_table.py (which invokes the former).

../src/experiments/vcr/make_table.py expects the recording to be extracted
in ./firefox-htable. To extract the recording this way, just run `tar xpf
firefox-table.tar.gz`.
