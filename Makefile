CFLAGS=-g -std=gnu11 -W -Wall
CC=gcc
SOURCES=*.c hash/*.c hash_array/*.c hash_hashtable/*.c test_hash/*.c

bin/main: $(SOURCES)
	$(CC) $(CFLAGS) $(SOURCES) -o $@
