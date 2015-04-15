#include "cobt/test.h"

#include "cobt/pma_test.h"
#include "cobt/tree_test.h"

void test_cobt(void) {
	test_cobt_tree();
	test_cobt_pma();
}
