CFLAGS=-g -std=gnu11 -W -Wall -O3
CC=gcc

SOURCES= \
	hash_array/*.c \
	hash_bplustree/*.c \
	hash_bplustree/private/*.c \
	hash_bplustree/test/*.c \
	hash/*.c test_hash/*.c \
	hash_hashtable/*.c \
	hash_hashtable/private/*.c \
	hash_hashtable/test/*.c \
	hash_splay/*.c \
	log/*.c \
	measurement/*.c \
	observation/*.c \
	observation/test/*.c \
	performance/*.c \
	rand/*.c \
	rand/test/*.c \
	splay_tree/*.c \
	stopwatch/*.c \
	util/*.c \

all: bin/test bin/performance

bin/test: test.c $(SOURCES)
	$(CC) $(CFLAGS) test.c $(SOURCES) -o $@

bin/performance: performance.c $(SOURCES)
	$(CC) $(CFLAGS) performance.c $(SOURCES) -o $@
