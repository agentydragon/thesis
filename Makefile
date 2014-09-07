CFLAGS=-g -std=gnu11 -W -Wall
CC=gcc
SOURCES= \
	hash/*.c test_hash/*.c \
	stopwatch/*.c \
	hash_array/*.c hash_hashtable/*.c \
	observation/*.c test_observation/*.c \
	performance/*.c

all: bin/main bin/hash_test bin/shakespeare

bin/main: main.c $(SOURCES)
	$(CC) $(CFLAGS) main.c $(SOURCES) -o $@

bin/hash_test: hash_test.c $(SOURCES)
	$(CC) $(CFLAGS) hash_test.c $(SOURCES) -o $@

bin/shakespeare: shakespeare/*.c $(SOURCES)
	$(CC) $(CFLAGS) shakespeare/*.c $(SOURCES) -o $@
