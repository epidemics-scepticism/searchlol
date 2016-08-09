#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <getopt.h>
#include <stdbool.h>
#include <pthread.h>
#include "search.h"
#include "onion.h"
#include "rsa.h"

void *s = NULL;
bool full = false;
size_t num_threads = 1;
pthread_t *threads = NULL;

void
die(void)
{
	if (s) {
		fprintf(stderr, "Cleaning up...\n");
		destroy_search(s);
	}
	for (pthread_t *p = threads; p && *p; p++) {
		pthread_cancel(*p);
	}
	fprintf(stderr, "Bye o/\n");
	_exit(0);
}

void
usage(void)
{

	fprintf(stderr, "Usage:\n");
	fprintf(stderr, "\t-d <path to dictionary> : load words from dictionary\n");
	fprintf(stderr, "\t-f : only match onions entirely populated by dictionary words\n");
	fprintf(stderr, "\t-t : number of threads\n");
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
	test_onions(arg->s, arg->full);
	return NULL;
}

int
main(int argc, char **argv) {
	signal(SIGINT, die);
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
	threads = calloc(num_threads+1, sizeof(pthread_t));
	if (!threads) {
		fprintf(stderr, "Allocation failed.\n");
		die();
	}
	struct thread_args arg = {
		.s = s,
		.full = full,
	};
	for (size_t i = 0; i < num_threads; i++) {
		fprintf(stderr, "Starting thread %u/%u...", i+1, num_threads);
		if (pthread_create(&threads[i], NULL, thread_shim, &arg)) {
			fprintf(stderr, "Failed\n");
			die();
		} else {
			fprintf(stderr, "Done\n");
		}
	}
	fprintf(stderr, "Starting search. Use ctrl-c to exit.\n");
	test_onions(s, full);
	die();
	return 0; /* lol jk */
}
