CFLAGS=-g -std=gnu11 -W -Wall -O2
CC=gcc

AD_SOURCES= \
	    ../../stopwatch/*.c ../../rand/*.c

access-distances: access-distances.c $(AD_SOURCES)
	$(CC) $(CFLAGS) access-distances.c $(AD_SOURCES) -o $@

asm:
	$(CC) $(CFLAGS) -S -fverbose-asm -c -std=gnu99 access-distances.c
