CFLAGS=-g -std=gnu11 -W -Wall -O3
CC=gcc

SOURCES= \
	hash/*.c test_hash/*.c \
	hash_hashtable/private/*.c hash_hashtable/test/*.c \
	stopwatch/*.c measurement/*.c \
	hash_array/*.c hash_hashtable/*.c \
	observation/*.c test_observation/*.c \
	rand/*.c rand/test/*.c \
	util/*.c \
	performance/*.c \
	test_hash_bplustree/*.c hash_bplustree/*.c hash_bplustree/private/*.c \

all: bin/test bin/performance

bin/test: test.c $(SOURCES)
	$(CC) $(CFLAGS) test.c $(SOURCES) -o $@

bin/performance: performance.c $(SOURCES)
	$(CC) $(CFLAGS) performance.c $(SOURCES) -o $@
