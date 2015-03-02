add_executable(veb_performance
	experiments/veb_performance/veb_performance.c
)
target_link_libraries(cob_cache_behavior LINK_PUBLIC hash_all m rt)
