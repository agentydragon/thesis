CFLAGS=-g -std=gnu11 -W -Wall -O3
CC=gcc

SOURCES= \
	hash/*.c test_hash/*.c \
	hash_hashtable/private/*.c \
	stopwatch/*.c measurement/*.c \
	hash_array/*.c hash_hashtable/*.c \
	observation/*.c test_observation/*.c \
	rand/*.c test_rand/*.c \
	util/*.c \
	performance/*.c

all: bin/test bin/performance bin/hash_test

bin/test: test.c $(SOURCES)
	$(CC) $(CFLAGS) test.c $(SOURCES) -o $@

bin/performance: performance.c $(SOURCES)
	$(CC) $(CFLAGS) performance.c $(SOURCES) -o $@

bin/hash_test: hash_test.c $(SOURCES)
	$(CC) $(CFLAGS) hash_test.c $(SOURCES) -o $@
