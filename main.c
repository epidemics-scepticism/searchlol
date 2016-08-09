#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <getopt.h>
#include <stdbool.h>
#include "search.h"
#include "onion.h"
#include "rsa.h"

void *s = NULL;
bool full = false;

void
die(void)
{
	if (s) {
		fprintf(stderr, "Cleaning up...\n");
		destroy_search(s);
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
}

bool
handle_args(int argc, char **argv)
{
	bool populated = false;
	int opt = -1;
	while ((opt = getopt(argc, argv, "fd:")) != -1) {
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
		default:
			usage();
			die();
		}
	}
	return populated;
}

int
main(int argc, char **argv) {
	if (argc < 2) {
	}
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
	fprintf(stderr, "Starting search. Use ctrl-c to exit.\n");
	test_onions(s, full);
	die();
	return 0; /* lol jk */
}
