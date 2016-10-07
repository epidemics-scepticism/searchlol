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

#include "onion.h"
#include "pronounce.h"
#include "search.h"
#include <err.h>
#include <openssl/pem.h>
#include <openssl/rand.h>
#include <openssl/rsa.h>
#include <openssl/sha.h>
#include <openssl/x509.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <sys/time.h>

static bool seeded_rng = false;

static size_t num_keys = 0;
static size_t num_matches = 0;
size_t start_time = 0;
static pthread_mutex_t stats_lock = PTHREAD_MUTEX_INITIALIZER;

static void seed_rng(void) {
  unsigned char buf[32] = {0};
  size_t read_bytes = 0;
  FILE *entropy = fopen("/dev/urandom", "r"); /* hardcoded urandom sucks */
  if (!entropy)
    err(-1, "fopen"); /* death before dishonour */
  while (read_bytes < sizeof(buf)) {
    size_t tmp = fread(&buf[read_bytes], 1, sizeof(buf) - read_bytes, entropy);
    if (tmp == 0 && (ferror(entropy) || feof(entropy))) {
      warn("fread");
    } else {
      read_bytes += tmp;
    }
  }
  while (!RAND_status()) {
    size_t tmp = fread(buf, 1, sizeof(buf), entropy);
    RAND_seed(buf, tmp);
  }
  fclose(entropy);
  RAND_seed(buf, sizeof(buf));
  seeded_rng = true;
}

static RSA *gen_rsa(void) {
  if (!RAND_status())
    seed_rng();
  int ret = 0;
  _Thread_local static int e_init = 0;
  _Thread_local static BIGNUM e;
  RSA *r = NULL;

  if (!e_init) {
    BN_init(&e);
    if (!BN_set_word(&e, 65537)) {
      warnx("BN_set_word");
      goto fail;
    }
    e_init = 1;
  }
  r = RSA_new();
  if (!r) {
    warnx("RSA_new");
    goto fail;
  }
  ret = RSA_generate_key_ex(r, 1024, &e, NULL);
  if (!ret) {
    warnx("RSA_generate_key");
    goto fail;
  }
  return r;
fail:
  if (r) {
    RSA_free(r);
    r = NULL;
  }
  return NULL;
}

static bool rsa_to_onion(RSA *r, char *o) {
  if (!r || !o)
    return false;
  unsigned char *bin = NULL;
  int i = i2d_RSAPublicKey(r, &bin);
  if (!bin)
    return false;
  if (i < 0)
    goto fail;
  unsigned char d[20] = {0};
  SHA1(bin, i, d);
  OPENSSL_free(bin);
  onion_encode(&o[0], &d[0]);
  o[16] = 0;
  return true;
fail:
  if (bin) {
    OPENSSL_free(bin);
  }
  return false;
}

void test_onions(const void *s, const bool full) {
  char o[17];
  RSA *r = NULL;
  FILE *out = NULL;
  while (true) {
    r = gen_rsa();
    if (!r)
      goto end_loop;
    pthread_mutex_lock(&stats_lock);
    num_keys++;
    pthread_mutex_unlock(&stats_lock);
    if (!rsa_to_onion(r, o))
      goto end_loop;
    if (search_search(s, o, full) || search_pronounce(o)) {
      warnx("found '%s'", o);
      pthread_mutex_lock(&stats_lock);
      num_matches++;
      pthread_mutex_unlock(&stats_lock);
      out = fopen(o, "w");
      if (!out) {
        warn("fopen");
        goto end_loop;
      }
      PEM_write_RSAPrivateKey(out, r, NULL, NULL, 0, NULL, NULL);
    }
  end_loop:
    if (r) {
      RSA_free(r);
      r = NULL;
    }
    if (out) {
      fclose(out);
      out = NULL;
    }
  }
}

size_t thetime(void) {
  struct timeval t;
  gettimeofday(&t, NULL);
  return (size_t)t.tv_sec;
}

void dump_stats(void) {
  size_t uptime = thetime() - start_time;
  if (start_time == 0 || uptime == 0)
    return;
  pthread_mutex_lock(&stats_lock);
  fprintf(stderr, "-- Stats --\nWe have been running for %lu seconds.\nWe have "
                  "generated %lu keys (%lu keys/second).\nWe have found %lu "
                  "matches.\n",
          uptime, num_keys, num_keys / uptime, num_matches);
  pthread_mutex_unlock(&stats_lock);
}
