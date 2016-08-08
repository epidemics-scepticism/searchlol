#ifndef _SEARCH_H
#define _SEARCH_H
#include <stdbool.h>
#include <stdint.h>
#include "onion.h"

void *new_search(void);

void destroy_search(void *_root);

bool populate_search(void *_root, const char *filename);

bool search_search(const void *_root, const u8 *onion, const bool full);
#endif
