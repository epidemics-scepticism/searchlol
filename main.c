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
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <getopt.h>
#include <stdbool.h>
#include <err.h>
#include <pthread.h>
#include <sys/wait.h>
#include "search.h"
#include "onion.h"
#include "rsa.h"
#include "lock.h"

void *s = NULL;
bool full = false;
size_t num_threads = 1;
pthread_t *threads = NULL;
pthread_t parent;

void
die(void)
{
	fprintf(stderr, "\rCleaning up...\n");
	if (threads) {
		for (size_t i = 0; i < num_threads; i++) {
			fprintf(stderr, "Stopping thread %u/%u...", i+1, num_threads);
			if (pthread_cancel(threads[i])) {
				fprintf(stderr, "Fail!\n");
			} else {
				pthread_join(threads[i], NULL);
				fprintf(stderr, "Done\n");
			}
		}
		free(threads);
	}
	fprintf(stderr, "Clearing search...");
	if (s)
		destroy_search(s);
	fprintf(stderr, "Done\n");
	fprintf(stderr, "Clearing locks...");
	lock_destroy();
	fprintf(stderr, "Done\n");
	_Exit(0);
}

void
usage(void)
{
	fprintf(stderr, "Usage:\n");
	fprintf(stderr, "\t-d <path to dictionary> : load words from dictionary\n");
	fprintf(stderr, "\t-f : only match onions entirely populated by dictionary words\n");
	fprintf(stderr, "\t-t <number>: number of threads\n");
}

bool
handle_args(int argc, char **argv)
{
	bool populated = false;
	int opt = -1;
	while ((opt = getopt(argc, argv, "fd:t:")) != -1) {
		switch (opt) {
		case 'f':
			full = true;
			break;
		case 'd':
			fprintf(stderr, "Populating search from '%s'\n", optarg);
			if (!populate_search(s, optarg)) {
				fprintf(stderr, "Fatal error while populating search\n");
				die();
			}
			populated = true;
			break;
		case 't':
			if (!sscanf(optarg, "%u", &num_threads) || !num_threads)
				num_threads = 1;
			break;
		default:
			usage();
			die();
		}
	}
	return populated;
}

struct thread_args {
	bool full;
	const void *s;
};

void *
thread_shim(void *_arg)
{
	struct thread_args *arg = _arg;
	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
	test_onions(arg->s, arg->full);
	return NULL;
}

extern size_t start_time;

void
sig_handle(int sig)
{
		if (sig == SIGINT) {
			die();
		} else if (sig == SIGUSR1) {
			dump_stats();
		}
}

int
main(int argc, char **argv) {
	s = new_search();
	if (!s) {
		fprintf(stderr, "Fatal error while allocating new search root\n");
		die();
	}
	if (!handle_args(argc, argv)) {
		fprintf(stderr, "No dictionaries supplied?\n");
		usage();
		die();
	}
	threads = calloc(num_threads, sizeof(pthread_t));
	if (!threads) {
		fprintf(stderr, "Allocation failed.\n");
		die();
	}
	struct thread_args arg = {
		.s = s,
		.full = full,
	};
	lock_create();
	start_time = thetime();
	for (size_t i = 0; i < num_threads; i++) {
		fprintf(stderr, "Starting thread %u/%u...", i+1, num_threads);
		if (pthread_create(&threads[i], NULL, thread_shim, &arg)) {
			warn("pthread_create");
			fprintf(stderr, "Failed\n");
			die();
		} else {
			fprintf(stderr, "Done\n");
		}
	}
	fprintf(stderr, "Starting search. Use ctrl-c to exit.\n");
	struct sigaction sa = {
		.sa_handler = sig_handle,
	};
	sigaction(SIGINT, &sa, NULL);
	sigaction(SIGUSR1, &sa, NULL);
	for (;;) {
		sleep(600);
		dump_stats();
	}
	return 0;
}
