/*
    Copyright (C) 2016 cacahuatl < cacahuatl at autistici dot org >

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "search.h"
#include <err.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct SEARCH_PROTO {
  bool is_word;
  struct SEARCH_PROTO **next;
};

typedef struct SEARCH_PROTO SEARCH;

void *new_search(void) {
  SEARCH *s = calloc(1, sizeof(SEARCH));
  if (!s) {
    warn("calloc");
    goto fail;
  }
  s->next = calloc(32, sizeof(SEARCH *));
  if (!s->next) {
    warn("calloc");
    goto fail;
  }
  s->is_word = false;
  return s;
fail:
  if (s) {
    if (s->next) {
      free(s->next);
      s->next = NULL;
    }
    free(s);
  }
  return NULL;
}

void destroy_search(void *_s) {
  SEARCH *s = _s;
  if (s) {
    if (s->next) {
      for (size_t i = 0; i < 32; i++) {
        destroy_search(s->next[i]);
      }
      free(s->next);
      s->next = NULL;
    }
    free(s);
    s = NULL;
  }
}

bool populate_search(void *_root, const char *filename) {
  SEARCH *root = _root;
  FILE *dict = NULL;
  if (!root)
    goto fail;
  if (!filename)
    goto fail;
  dict = fopen(filename, "r");
  if (!dict)
    goto fail;
  unsigned char buf[512] = {0};
  while (fgets(buf, sizeof(buf), dict)) {
    char *ptr = strchr(buf, 10);
    if (ptr)
      *ptr = 0;
    size_t len = strlen(buf);
    if (len < 3) {
      warnx("'%s' is %lu characters long, skipping...", buf, len);
      continue;
    }
    if (!onion_only(buf)) {
      warnx("'%s' contains non-onion characters, skipping...", buf);
      continue;
    }
    SEARCH *s = root;
    for (size_t i = 0; i < len; i++) {
      size_t o = onion_values[buf[i]];
      if (!s->next[o]) {
        s->next[o] = new_search();
        if (!s->next[o]) {
          warn("calloc");
          goto fail;
        }
      }
      s = s->next[o];
    }
    s->is_word = true;
  }
  fclose(dict);
  return true;
fail:
  destroy_search(root); /* might end up in a half-populated state...ugly */
  if (dict)
    fclose(dict);
  return false;
}

bool search_search(const void *_root, const unsigned char *onion,
                   const bool full) {
  const SEARCH *root = _root;
  if (!root)
    goto fail;
  if (!onion)
    goto fail;
  if (!onion_only(onion))
    goto fail;
  size_t len = strlen(onion);
  const SEARCH *s = root;
  for (size_t i = 0; i < len; i++) {
    if (s->is_word) {
      if (!full)
        return true;
      else if (search_search(root, &onion[i], full))
        return true;
    }
    size_t o = onion_values[onion[i]];
    if (s->next[o])
      s = s->next[o];
    else
      return false;
  }
  return true;
fail:
  return false;
}
