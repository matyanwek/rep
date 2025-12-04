rep: main.c
	cc -o rep main.c

install: rep rep.1
	mkdir -p /usr/local/bin
	cp rep /usr/local/bin
	mkdir -p /usr/local/share/man/man1
	cp rep.1 /usr/local/share/man/man1

test: rep
	./test.sh

clean: rep
	rm rep
