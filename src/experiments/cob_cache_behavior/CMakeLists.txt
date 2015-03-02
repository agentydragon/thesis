add_executable(cob_cache_behavior
	experiments/cob_cache_behavior/cob_cache_behavior.c
	test_hash/toycrypt.c
	stopwatch/stopwatch.c
	measurement/measurement.c
	rand/rand.c
)
target_link_libraries(cob_cache_behavior LINK_PUBLIC hash_all m rt)
