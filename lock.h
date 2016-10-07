#pragma once
#include <openssl/crypto.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdlib.h>

bool lock_create(void);

void lock_destroy(void);
