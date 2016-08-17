#pragma once
#include <stdbool.h>
#include <pthread.h>
#include <openssl/crypto.h>
#include <stdlib.h>

bool
lock_create(void);

void
lock_destroy(void);
