#pragma once
#include <stdbool.h>
#include <stdint.h>

/* word is a nul terminated string, returns true if only onion characters */
bool onion_only(const unsigned char *word);
/* dst is 10 bytes, src is 16 bytes */
void onion_decode(unsigned char *dst, const unsigned char *src);
/* dst is 16 bytes, src is 10 bytes */
void onion_encode(unsigned char *dst, const unsigned char *src);

extern const unsigned char onion_chars[32];
extern const unsigned char onion_values[256];
