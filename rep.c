#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typedef struct {
	size_t cap;
	size_t len;
	char *data;
} StringBuilder;

StringBuilder *sb_init() {
	StringBuilder *sb = malloc(sizeof(StringBuilder));
	if (sb == NULL) {
		char *msg = strerror(errno);
		fprintf(stderr, "%s\n", msg);
		exit(EXIT_FAILURE);
	}

	sb->cap = 0;
	sb->len = 0;
	return sb;
}

void sb_del(StringBuilder *sb) {
	free(sb->data);
	free(sb);
}

void *sb_extend(StringBuilder *sb, const char *s, size_t len) {
	if (sb->cap < sb->len + len) {
		sb->cap = sb->len + len;
		sb->data = realloc(sb->data, sb->cap * sizeof(char));
		if (sb->data == NULL) {
			char *msg = strerror(errno);
			fprintf(stderr, "%s\n", msg);
			exit(EXIT_FAILURE);
		}
	}

	memcpy(sb->data + sb->len, s, len);
	sb->len += len;

	return sb;
}

bool load_stdin(StringBuilder *sb) {
	char buf[BUFSIZ];

	for (;;) {
		size_t read_len = fread(buf, sizeof(char), BUFSIZ, stdin);
		sb_extend(sb, buf, read_len);

		if (read_len != BUFSIZ * sizeof(char)) {
			if (ferror(stdin)) {
				fprintf(stderr, "error reading STDIN\n");
				return false;
			}
			break;
		}
	}

	return true;
}

void dump_stdin(const StringBuilder *sb) {
	size_t write_len = fwrite(sb->data, sizeof(char), sb->len, stdout);
	if (write_len < sb->len) {
		fprintf(stderr, "error writing to STDOUT\n");
		exit(EXIT_FAILURE);
	}
}

bool dump_file(const char *filename) {
	FILE *file = fopen(filename, "r");
	if (file == NULL) {
		char *msg = strerror(errno);
		fprintf(stderr, "error opening file '%s': %s\n", filename, msg);
		return false;
	}

	char buf[BUFSIZ];
	for (;;) {
		size_t read_len = fread(buf, sizeof(char), BUFSIZ, file);
		size_t write_len = fwrite(buf, sizeof(char), read_len, stdout);

		if (write_len < read_len) {
			fprintf(stderr, "error writing to STDOUT\n");
			exit(EXIT_FAILURE);
		}

		if (read_len < BUFSIZ) {
			if (ferror(file)) {
				fprintf(stderr, "error reading file '%s'\n", filename);
				return false;
			}

			break;
		}
	}

	if (fclose(file) != 0) {
		char *msg = strerror(errno);
		fprintf(stderr, "error closing file '%s': %s\n", filename, msg);
		return false;
	}

	return true;
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
	int arglen = argc - optind;

	StringBuilder *sb = sb_init();

	if (arglen < 1) {
		// no file arguments; rep stdin and exit early;
		load_stdin(sb);
		for (int i = 0; i < nflag; i++) {
			dump_stdin(sb);
		}
		return EXIT_SUCCESS;
	}

	bool file_ok[arglen];
	for (int i = 0; i < arglen; i++) {
		file_ok[i] = true;
	}

	int ret = EXIT_SUCCESS; // return failure if we get an error reading a file
	bool stdin_loaded = false;
	for (int i = 0; i < nflag; i++) {
		for (int j = 0; j < arglen; j++) {
			char *filename = argv[optind + j];

			if (!file_ok[j]) {
				// previous error reading file; skip
				continue;
			} else if (strcmp(filename, "-") != 0) {
				// file; dump
				if (!dump_file(filename)) {
					file_ok[j] = false;
					ret = EXIT_FAILURE;
				}
			} else {
				// stdin; load if needed, then dump
				if (!stdin_loaded) {
					if (load_stdin(sb)) {
						stdin_loaded = true;
					} else {
						file_ok[j] = false;
						ret = EXIT_FAILURE;
						continue;
					}
				}
				dump_stdin(sb);
			}
		}
	}

	return ret;
}
