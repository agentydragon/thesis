#ifndef DICT_REGISTER_H
#define DICT_REGISTER_H

#include "dict/dict.h"

extern const dict_api* DICT_API_REGISTER[];

const dict_api* dict_api_find(const char* name);
void dict_api_list_parse(char* name_list, dict_api const ** list,
		uint64_t capacity);

#endif
