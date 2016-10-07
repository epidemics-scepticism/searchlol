#include "pronounce.h"
#include <err.h>
#include <stdlib.h>

struct PRONOUNCE_PROTO {
  bool end;
  struct PRONOUNCE_PROTO **next;
};

typedef struct PRONOUNCE_PROTO PRONOUNCE;

static const unsigned char *vowels[] = {
    "a", "e", "i", "o",  "u",  "a",  "e",  "i",  "o",  "u",  "a",
    "e", "i", "o", "ea", "ou", "ie", "ai", "ee", "au", "oo", NULL,
};

static const unsigned char *consonants[] = {
    "b",  "c",  "d",  "g",  "h",  "j",  "k",  "l",  "m",  "n",  "p",  "r",
    "s",  "t",  "v",  "w",  "y",  "z",  "tr", "cl", "cr", "br", "fr", "th",
    "dr", "ch", "st", "sp", "sw", "pr", "sh", "gr", "tw", "wr", "ck", NULL,
};

static PRONOUNCE *new_pronounce_internal(void) {
  PRONOUNCE *p = calloc(1, sizeof(PRONOUNCE));
  if (!p) {
    warn("calloc");
    goto fail;
  }
  p->next = calloc(32, sizeof(PRONOUNCE *));
  if (!p->next) {
    warn("calloc");
    goto fail;
  }
  return p;
fail:
  if (p) {
    if (p->next) {
      free(p->next);
      p->next = NULL;
    }
    free(p);
    p = NULL;
  }
  return NULL;
}

PRONOUNCE *pro_cons = NULL;
PRONOUNCE *pro_vows = NULL;

static bool populate_pronounce_internal(PRONOUNCE *root,
                                        const unsigned char **syl) {
  if (!root)
    return false;
  root->end = false;
  for (const unsigned char **s = syl; *s; s++) {
    PRONOUNCE *p = root;
    for (const unsigned char *c = *s; *c; c++) {
      unsigned char v = onion_values[*c];
      if (!p->next[v]) {
        p->next[v] = new_pronounce_internal();
        if (!p->next[v])
          goto fail;
      }
      p = p->next[v];
    }
    p->end = true;
  }
  return true;
fail:
  return false;
}

bool populate_pronounce(void) {
  pro_cons = new_pronounce_internal();
  if (!pro_cons) {
    goto fail;
  }
  pro_vows = new_pronounce_internal();
  if (!pro_vows) {
    goto fail;
  }
  if (!populate_pronounce_internal(pro_cons, consonants)) {
    goto fail;
  }
  if (!populate_pronounce_internal(pro_vows, vowels)) {
    goto fail;
  }
  return true;
fail:
  return false;
}

static bool search_pronounce_internal(PRONOUNCE *p, const unsigned char *o,
                                      bool cons) {
  for (const char *c = o; *c; c++) {
    if (p->end) {
      if (cons && search_pronounce_internal(pro_vows, c, false))
        return true;
      if (!cons && search_pronounce_internal(pro_cons, c, true))
        return true;
    }
    unsigned char v = onion_values[*c];
    if (v < 0x20 && p->next[v])
      p = p->next[v];
    else
      return false;
  }
  return p->end;
}

bool search_pronounce(const char *o) {
  if (search_pronounce_internal(pro_cons, o, true) ||
      search_pronounce_internal(pro_vows, o, false))
    return true;
  return false;
}

static void destroy_pronounce_internal(PRONOUNCE *p) {
  if (p->next) {
    for (size_t i = 0; i < 32; i++) {
      if (p->next[i]) {
        destroy_pronounce_internal(p->next[i]);
        free(p->next[i]);
        p->next[i] = NULL;
      }
    }
    free(p->next);
    p->next = NULL;
  }
}

void destroy_pronounce(void) {
  if (pro_cons) {
    destroy_pronounce_internal(pro_cons);
    free(pro_cons);
    pro_cons = NULL;
  }
  if (pro_vows) {
    destroy_pronounce_internal(pro_vows);
    free(pro_vows);
    pro_vows = NULL;
  }
}
