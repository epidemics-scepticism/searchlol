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

#include "lock.h"

static pthread_mutex_t *muts = NULL;
static int num_locks = 0;

void
lock_handler(int mode, int n, const char *file, int line)
{
	if (0 > n || num_locks < n)
		return;
	if (mode & CRYPTO_LOCK) {
		pthread_mutex_lock(&muts[n]);
	} else {
		pthread_mutex_unlock(&muts[n]);
	}
}

bool
lock_create(void)
{
	if (!muts) {
		num_locks = CRYPTO_num_locks();
		if (num_locks < 0) return false;
		muts = calloc(num_locks, sizeof(pthread_mutex_t));
		if (!muts) return false;
		for (int i = 0; i < num_locks; i++)
			pthread_mutex_init(&muts[i], NULL);
		CRYPTO_set_id_callback(pthread_self);
		CRYPTO_set_locking_callback(lock_handler);
		return true;
	}
	return false;
}

void
lock_destroy(void)
{
	if (muts) {
		for (size_t i = 0; i < num_locks; i++) {
			pthread_mutex_destroy(&muts[i]);
		}
		free(muts);
		muts = NULL;
	}
}
