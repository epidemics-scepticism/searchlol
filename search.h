#pragma once
#include <stdbool.h>
#include <stdint.h>
#include "onion.h"

void *new_search(void);

void destroy_search(void *_root);

bool populate_search(void *_root, const char *filename);

bool search_search(const void *_root, const unsigned char *onion, const bool full);
