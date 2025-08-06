#include <errno.h>
#include <error.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typedef struct {
	size_t cap;
	size_t len;
	char *data;
} String;

void *string_grow(String *s, size_t delta) {
	size_t new_cap = s->cap > 0 ? s->cap : delta;
	while (new_cap < s->len + delta) {
		new_cap *= 2;
	}

	if (s->cap < new_cap) {
		s->data = realloc(s->data, sizeof(char) * new_cap);
		if (s->data == NULL) {
			error(EXIT_FAILURE, errno, "error growing String");
		}
		s->cap = new_cap;
	}

	return s;
}

bool read_stdin(String *s) {
	for (;;) {
		string_grow(s, BUFSIZ);
		size_t n = fread(s->data + s->len, sizeof(char), BUFSIZ, stdin);
		s->len += n;
		if (n == BUFSIZ) {
			continue;
		} else if (ferror(stdin)) {
			return false;
		} else {
			return true;
		}
	}
}

bool dump_string(const String *s) {
	size_t n = fwrite(s->data, sizeof(char), s->len, stdout);
	return n == s->len || !(bool)ferror(stdout);
}

bool dump_file(const char *filename) {
	FILE *file;
	file = fopen(filename, "r");
	if (file == NULL) {
		return false;
	}

	char buf[BUFSIZ];
	for (;;) {
		size_t n_read = fread(buf, sizeof(char), BUFSIZ, file);
		size_t n_wrote = fwrite(buf, sizeof(char), n_read, stdout);

		if (n_wrote == BUFSIZ) {
			continue;
		} else if (n_wrote < n_read && ferror(stdout)) {
			fclose(file);
			return false;
		} else {
			bool ok = !ferror(file) && !(bool)fclose(file);
			return ok;
		}
	}
}

int main(int argc, char **argv) {
	int opt;
	int nflag = 10;
	while ((opt = getopt(argc, argv, "n:h")) != -1) {
		switch (opt) {
		case 'n':
			nflag = atoi(optarg);
			if (nflag <= 0) {
				fprintf(stderr, "invalid -n flag: %s\n", optarg);
				exit(EXIT_FAILURE);
			}
			break;
		case 'h':
			printf("Usage: %s [OPTION]... [FILE]...\n", argv[0]);
			printf("Output the concatenation of input FILEs n times (10 by default).\n");
			printf("\n");
			printf("With no FILE, or when FILE is -, output STDIN.\n");
			printf("\n");
			printf("\t-n\tNumber of times to repeat input files (10 by default)\n");
			printf("\t-h\tprint this Help and exit\n");
			exit(EXIT_SUCCESS);
			break;
		default: /* '?' */
			fprintf(stderr, "Usage: %s [OPTION]... [FILE]...\n", argv[0]);
			fprintf(stderr, "try %s -h for help\n", argv[0]);
			exit(EXIT_FAILURE);
			break;
		}
	}

	size_t nargs = argc - optind;
	char **args;
	if (nargs < 1) {
		nargs = 1;
		args = (char *[]){"-"};
	} else {
		args = argv + optind;
	}

	String stdin_buf;
	bool got_stdin = false;

	for (int i = 0; i < nflag; i++) {
		for (size_t i = 0; i < nargs; i++) {
			char *filename = args[i];

			if (strcmp(filename, "-") != 0) {
				if (!dump_file(filename)) {
					error(EXIT_FAILURE, errno, "error reading from \"%s\"", filename);
				}
				continue;
			}

			if (!got_stdin) {
				if (!read_stdin(&stdin_buf)) {
					error(EXIT_FAILURE, errno, "error reading from stdin");
				}
				got_stdin = true;
			}

			if (!dump_string(&stdin_buf)) {
				error(EXIT_FAILURE, errno, "error writing to stdout");
			}
		}
	}

	return 0;
}
