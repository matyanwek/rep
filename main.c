#include <ctype.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define arrlen(arr) (sizeof(arr) / sizeof(*(arr)))

#define Void void
typedef bool Bool;
typedef unsigned char Byte;
typedef ssize_t Int;

Int parseNum(Byte *s) {
	Int n = 0;
	for (; *s; s++) {
		if (!isdigit(*s)) {
			return 0;
		}
		n *= 10;
		n += *s - '0';
	}
	return n;
}

typedef struct {
	Byte *data;
	Int len;
	Int cap;
} Sponge;

Bool extend(Sponge *sponge, Byte *s, Int n) {
	if (sponge->cap == 0) {
		sponge->cap = 1;
	}

	while (sponge->cap < sponge->len + n) {
		sponge->cap *= 2;
	}

	sponge->data = realloc(sponge->data, sponge->cap);
	if (sponge->data == NULL) {
		return false;
	}

	memcpy(sponge->data + sponge->len, s, n);
	sponge->len += n;
	return true;
}

Bool soak(Sponge *sponge) {
	Byte buf[BUFSIZ];
	Int n = sizeof(buf);
	while (n >= sizeof(buf)) {
		n = fread(buf, 1, sizeof(buf), stdin);
		Bool ok = extend(sponge, buf, n);
		if (!ok) {
			return false;
		}
	}
	return !ferror(stdin);
}

Bool dump(Sponge sponge) {
	fwrite(sponge.data, 1, sponge.len, stdout);
	return !ferror(stdout);
}

Bool repeat(Byte *filename) {
	FILE *f = fopen(filename, "r");
	if (f == NULL) {
		return false;
	}

	Byte buf[BUFSIZ];
	Int read = sizeof(buf);
	Int wrote = read;
	while (read >= sizeof(buf) && wrote == read) {
		read = fread(buf, 1, sizeof(buf), f);
		wrote = fwrite(buf, 1, read, stdout);
	}
	Bool ok = !ferror(f) && !ferror(stdout);

	Int oldErrno = errno;
	Bool closed = !fclose(f);
	if (!ok) {
		errno = oldErrno;
	}
	return ok && closed;
}

#define usage "Usage: %s [OPTION]... [FILE]...\n"

#define help \
	"Output the concatenation of input FILEs n times (10 by default).\n" \
	"\n" \
	"With no FILE, or when FILE is -, output STDIN.\n" \
	"\n" \
	"\t-n\tNumber of times to repeat input files (10 by default)\n" \
	"\t-h\tprint this Help and exit\n" \

#define errmsg(...) do { \
		fprintf(stderr, "%s: ", argv[0]); \
		fprintf(stderr, __VA_ARGS__); \
		if (errno) { \
			fprintf(stderr, ": %s\n", strerror(errno)); \
		} else { \
			fprintf(stderr, "\n"); \
		} \
	} while (0);

int main(int argc, char **argv) {
	Int opt;
	Int count = 10;
	while ((opt = getopt(argc, argv, "n:h")) != -1) {
		switch (opt) {
		case 'n':
			count = parseNum(optarg);
			if (count < 1) {
				errmsg("invalid -n flag: %s\n", optarg)
				return 1;
			}
			break;
		case 'h':
			printf(usage, argv[0]);
			printf(help);
			return 0;
		default:
			fprintf(stderr, usage, argv[0]);
			fprintf(stderr, "try %s -h for help\n", argv[0]);
			return 1;
		}
	}

	Byte *altArgv[] = {argv[0], "-"};
	if (argc - optind < 1) {
		argv = (char **)altArgv;
		argc = arrlen(altArgv);
		optind = 1;
	}

	Bool ok;
	Sponge sponge = {0};

	while (count-- > 0) {
		for (Int i = optind; i < argc; i++) {
			if (strcmp(argv[i], "-") != 0) {
				ok = repeat(argv[i]);
				if (!ok) {
					errmsg("error repeating \"%s\"", argv[i]);
					return 1;
				}
				continue;
			}

			if (sponge.len < 1) {
				ok = soak(&sponge);
				if (!ok) {
					errmsg("error soaking standard input");
					return 1;
				}
			}

			ok = dump(sponge);
			if (!ok) {
				errmsg("error repeating standard input");
				return 1;
			}
		}
	}

	return 0;
}
