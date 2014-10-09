#include "test.h"
#include "find.h"
#include "insert.h"
#include "delete.h"

void test_hash_bplustree() {
	test_hash_bplustree_insert();
	test_hash_bplustree_delete();
	test_hash_bplustree_find();
}
