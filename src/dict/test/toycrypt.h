#ifndef DICT_TEST_TOYCRYPT_H
#define DICT_TEST_TOYCRYPT_H

#include <stdint.h>

uint64_t toycrypt(uint64_t message, uint64_t key);
uint64_t toyuncrypt(uint64_t message, uint64_t key);

#endif
