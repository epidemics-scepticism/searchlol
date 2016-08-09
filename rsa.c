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
#include <stdio.h>
#include <stdbool.h>
#include <err.h>
#include <pthread.h>
#include <openssl/pem.h>
#include <openssl/rsa.h>
#include <openssl/rand.h>
#include <openssl/sha.h>
#include <openssl/x509.h>
#include "search.h"
#include "onion.h"

static bool seeded_rng = false;

static void
seed_rng(void)
{
	unsigned char buf[32] = { 0 };
	size_t read_bytes = 0;
	FILE *entropy = NULL;
	entropy = fopen("/dev/urandom", "r"); /* hardcoded urandom sucks */
	if (!entropy)
		err(-1, "fopen"); /* death before dishonour */
	while (read_bytes < sizeof(buf)) {
		size_t tmp = fread(&buf[read_bytes], 1, sizeof(buf) - read_bytes, entropy);
		if (tmp == 0 && (ferror(entropy)||feof(entropy))) {
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

static RSA *
gen_rsa(pthread_mutex_t *lock)
{
	if (!RAND_status())
		seed_rng();
	int ret = -1;
	BIGNUM *e = NULL;
	RSA *r = NULL;

	pthread_mutex_lock(lock);
	e = BN_new();
	pthread_mutex_unlock(lock);
	if (!e) {
		warnx("BN_new");
		goto fail;
	}
	pthread_mutex_lock(lock);
	BN_init(e);
	pthread_mutex_unlock(lock);
	if (!BN_set_word(e, 65537)) {
		warnx("BN_set_word");
		goto fail;
	}
	pthread_mutex_lock(lock);
	r = RSA_new();
	pthread_mutex_unlock(lock);
	if (!r) {
		warnx("RSA_new");
		goto fail;
	}
	pthread_mutex_lock(lock);
	ret = RSA_generate_key_ex(r, 1024, e, NULL);
	pthread_mutex_unlock(lock);
	if (!ret) {
		warnx("RSA_generate_key");
		goto fail;
	}
	if (e) {
		pthread_mutex_lock(lock);
		BN_free(e);
		pthread_mutex_unlock(lock);
		e=NULL;
	}
	return r;
fail:
	if (e) {
		pthread_mutex_lock(lock);
		BN_free(e);
		pthread_mutex_unlock(lock);
		e=NULL;
	}
	if (r) {
		pthread_mutex_lock(lock);
		RSA_free(r);
		pthread_mutex_unlock(lock);
		r=NULL;
	}
	return NULL;
}

static bool
rsa_to_onion(RSA *r, char *o, pthread_mutex_t *lock)
{
	if (!r || !o)
		return false;
	unsigned char *bin = NULL;
	pthread_mutex_lock(lock);
	int i = i2d_RSAPublicKey(r, &bin);
	pthread_mutex_unlock(lock);
	if (!bin)
		return false;
	if (i < 0)
		goto fail;
	unsigned char d[20] = { 0 };
	pthread_mutex_lock(lock);
	SHA1(bin, i, d);
	pthread_mutex_unlock(lock);
	pthread_mutex_lock(lock);
	OPENSSL_free(bin);
	pthread_mutex_unlock(lock);
	onion_encode(&o[0], &d[0]);
	o[16] = 0;
	return true;
fail:
	if (bin) {
		pthread_mutex_lock(lock);
		OPENSSL_free(bin);
		pthread_mutex_unlock(lock);
	}
	return false;
}

void
test_onions(const void *s, const bool full, pthread_mutex_t *lock)
{
	seed_rng();
	char o[17];
	RSA *r = NULL;
	FILE *out = NULL;
	while (true) {
		r = gen_rsa(lock);
		if (!r)
			goto end_loop;
		if (!rsa_to_onion(r, o, lock))
			goto end_loop;
		if (search_search(s, o, full)) {
			warnx("found '%s'", o);
			out = fopen(o, "w");
			if (!out) {
				warn("fopen");
				goto end_loop;
			}
			pthread_mutex_lock(lock);
			PEM_write_RSAPrivateKey(out, r, NULL, NULL, 0, NULL, NULL);
			pthread_mutex_unlock(lock);
		}
end_loop:
		if (r) {
			pthread_mutex_lock(lock);
			RSA_free(r);
			pthread_mutex_unlock(lock);
			r=NULL;
		}
		if (out) {fclose(out);out=NULL;}
	}
}
