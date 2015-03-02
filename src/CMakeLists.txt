cmake_minimum_required (VERSION 2.6)
project(Bakalarka)

# (Profiling)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pg")
set(CMAKE_EXE_LINKER_FLAGS "-pg")
set(CMAKE_BUILD_TYPE "Debug")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c11 -D_GNU_SOURCE")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}")

add_library(hash_all
	btree/btree.c
	cache_oblivious_btree/cache_oblivious_btree.c
	hash_array/hash_array.c
	hash_btree/hash_btree.c
	hash_cobt/hash_cobt.c
	hash/hash.c
	hash_hashtable/hash_hashtable.c
	hash_hashtable/private/delete.c
	hash_hashtable/private/dump.c
	hash_hashtable/private/find.c
	hash_hashtable/private/hash.c
	hash_hashtable/private/insertion.c
	hash_hashtable/private/resizing.c
	hash_hashtable/private/traversal.c
	hash_splay/hash_splay.c
	log/log.c
	math/math.c
	ordered_file_maintenance/ordered_file_maintenance.c
	splay_tree/splay_tree.c
	util/average.c
	veb_layout/veb_layout.c
)
include(${CMAKE_CURRENT_SOURCE_DIR}/experiments/cob_cache_behavior/CMakeLists.txt)

add_executable(test_bin
	btree/test.c
	cache_oblivious_btree/test.c
	hash_hashtable/test/delete.c
	hash_hashtable/test/find.c
	hash_hashtable/test/helper.c
	hash_hashtable/test/test.c
	math/test.c
	observation/observation.c
	observation/test/test.c
	ordered_file_maintenance/test.c
	rand/rand.c
	rand/test/test.c
	splay_tree/test.c
	test.c
	test/hash_blackbox.c
	test_hash/test_hash_large.c
	test_hash/toycrypt.c
	test/ordered_hash_blackbox.c
	veb_layout/test.c
)
target_link_libraries(test_bin LINK_PUBLIC hash_all m rt)

add_executable(performance_bin
	performance.c
	performance/random_read.c
	test_hash/test_hash_large.c
	test_hash/toycrypt.c
	stopwatch/stopwatch.c
	measurement/measurement.c
	rand/rand.c
)
target_link_libraries(performance_bin LINK_PUBLIC hash_all m rt)
