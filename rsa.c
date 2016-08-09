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
gen_rsa(void)
{
	if (!RAND_status())
		seed_rng();
	unsigned long len = 1024;
	BIGNUM *e = NULL;
	RSA *r = NULL;

	e = BN_new();
	if (!e) {
		warnx("BN_new");
		goto fail;
	}
	if (!BN_set_word(e, 65537)) {
		warnx("BN_set_word");
		goto fail;
	}
	r = RSA_new();
	if (!r) {
		warnx("RSA_new");
		goto fail;
	}
	if (!RSA_generate_key_ex(r, len, e, NULL)) {
		warnx("RSA_generate_key");
		goto fail;
	}
	if (e) {BN_free(e);e=NULL;}
	return r;
fail:
	if (e) {BN_free(e);e=NULL;}
	if (r) {RSA_free(r);r=NULL;}
	return NULL;
}

static bool
rsa_to_onion(RSA *r, char *o)
{
	if (!r || !o)
		return false;
	unsigned char *bin = NULL;
	int i = i2d_RSAPublicKey(r, &bin);
	if (!bin)
		return false;
	if (i < 0)
		goto fail;
	unsigned char d[20];
	SHA1(bin, i, d);
	OPENSSL_free(bin);
	onion_encode(&o[0], &d[0]);
	o[16] = 0;
	return true;
fail:
	if (bin) OPENSSL_free(bin);
	return false;
}

void
test_onions(const void *s, const bool full)
{
	seed_rng();
	char o[17];
	RSA *r = NULL;
	FILE *out = NULL;
	while (true) {
		r = gen_rsa();
		if (!r)
			goto end_loop;
		if (!rsa_to_onion(r,o))
			goto end_loop;
		if (search_search(s, o, full)) {
			warnx("found '%s'", o);
			out = fopen(o, "w");
			if (!out) {
				warn("fopen");
				goto end_loop;
			}
			PEM_write_RSAPrivateKey(out, r, NULL, NULL, 0, NULL, NULL);
		}
end_loop:
		if (r) {RSA_free(r);r=NULL;}
		if (out) {fclose(out);out=NULL;}
	}
}
