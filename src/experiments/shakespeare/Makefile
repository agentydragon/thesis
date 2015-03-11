CFLAGS=-g -std=gnu11 -W -Wall -O3
CC=gcc

SOURCES = \
	  ../../experiments/shakespeare/*.c ../../hash/*.c \
	  ../../dict_htable/*.c ../../dict_htable/private/*.c \
	  ../../stopwatch/*.c ../../util/*.c

shakespeare: $(SOURCES)
	$(CC) $(CFLAGS) $(SOURCES) -o $@
