CFLAGS=-g -std=gnu11 -W -Wall -O3
CC=gcc

SOURCES = \
	  ../../experiments/shakespeare/*.c ../../hash/*.c \
	  ../../hash_hashtable/*.c ../../hash_hashtable/private/*.c \
	  ../../stopwatch/*.c ../../util/*.c

shakespeare: $(SOURCES)
	$(CC) $(CFLAGS) $(SOURCES) -o $@
