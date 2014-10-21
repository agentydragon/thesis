CFLAGS=-g -std=gnu11 -W -Wall -O0
CC=gcc

AD_SOURCES= \
	    ../../stopwatch/*.c ../../rand/*.c
MAIN=random-vs-sequential.c

random-vs-sequential: $(MAIN) $(AD_SOURCES)
	$(CC) $(CFLAGS) $(MAIN) $(AD_SOURCES) -o $@

asm:
	$(CC) $(CFLAGS) -S -fverbose-asm -c -std=gnu99 $(MAIN)
