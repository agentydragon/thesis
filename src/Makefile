# -pedantic-errors
CFLAGS=-std=gnu11 -W -Wall -O3 -rdynamic -lm
CC=gcc


SOURCES= \
	cache_oblivious_btree/*.c \
	stopwatch/*.c measurement/*.c \
	hash/*.c test_hash/*.c \
	hash_array/*.c \
	hash_bplustree/*.c hash_bplustree/private/*.c hash_bplustree/test/*.c \
	hash_cobt/*.c \
	hash_hashtable/*.c hash_hashtable/private/*.c hash_hashtable/test/*.c \
	observation/*.c observation/test/*.c \
	ordered_file_maintenance/*.c \
	log/*.c \
	math/*.c \
	performance/*.c \
	rand/*.c rand/test/*.c \
	test/*.c \
	util/*.c \
	veb_layout/*.c

bin/test: test.c $(SOURCES)
	$(CC) -g $(CFLAGS) test.c $(SOURCES) -o $@

bin/test_profile: test.c $(SOURCES)
	$(CC) $(CFLAGS) -pg test.c $(SOURCES) -o $@

asm: test.c $(SOURCES)
	$(CC) $(CFLAGS) -S -fverbose-asm test.c $(SOURCES)

bin/performance: performance.c $(SOURCES)
	$(CC) $(CFLAGS) performance.c $(SOURCES) -o $@

bin/experiments/cob_cache_behavior: experiments/cob_cache_behavior/cob_cache_behavior.c $(SOURCES)
	$(CC) $(CFLAGS) -pg experiments/cob_cache_behavior/cob_cache_behavior.c $(SOURCES) -o $@

bin/experiments/cob_cache_behavior_profile: experiments/cob_cache_behavior/cob_cache_behavior.c $(SOURCES)
	$(CC) $(CFLAGS) experiments/cob_cache_behavior/cob_cache_behavior.c $(SOURCES) -o $@

bin/experiments/veb_performance: experiments/veb_performance/veb_performance.c $(SOURCES)
	$(CC) $(CFLAGS) -pg experiments/veb_performance/veb_performance.c $(SOURCES) -o $@
