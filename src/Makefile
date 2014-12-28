CFLAGS=-g -std=gnu11 -W -Wall -O3 -rdynamic -lm
CC=gcc

SOURCES= \
	cache_oblivious_btree/*.c \
	stopwatch/*.c measurement/*.c \
	hash/*.c test_hash/*.c \
	hash_array/*.c \
	hash_bplustree/*.c hash_bplustree/private/*.c hash_bplustree/test/*.c \
	hash_hashtable/*.c hash_hashtable/private/*.c hash_hashtable/test/*.c \
	observation/*.c observation/test/*.c \
	ordered_file_maintenance/*.c \
	log/*.c \
	math/*.c \
	performance/*.c \
	rand/*.c rand/test/*.c \
	util/*.c \
	veb_layout/*.c

all: bin/test bin/performance

bin/test: test.c $(SOURCES)
	$(CC) $(CFLAGS) test.c $(SOURCES) -o $@

bin/performance: performance.c $(SOURCES)
	$(CC) $(CFLAGS) performance.c $(SOURCES) -o $@
