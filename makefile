.PHONY: all

all:
	cc -o rep rep.c

.PHONY: install

install:
	mkdir -p /usr/local/bin
	mkdir -p /usr/local/share/man/man1
	cc -o rep rep.c
	cp rep /usr/local/bin
	cp rep.1 /usr/local/share/man/man1

.PHONY: test

test:
	cc -o rep rep.c && ./test.sh
