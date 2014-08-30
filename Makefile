CFLAGS=-g -std=gnu11 -W -Wall
CC=gcc
SOURCES=*.c \
	hash/*.c test_hash/*.c \
	hash_array/*.c hash_hashtable/*.c \
	observation/*.c test_observation/*.c

bin/main: $(SOURCES)
	$(CC) $(CFLAGS) $(SOURCES) -o $@
