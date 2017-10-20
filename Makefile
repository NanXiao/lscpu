# lscpu Makefile
# Public Domain

CC ?=		cc
CFLAGS ?=	-g -O2 -Wall
PREFIX ?=	/usr/local

all:
	${CC} ${CFLAGS} ${LDFLAGS} -o lscpu lscpu.c

install:
	install -c -s -m 555 lscpu ${PREFIX}/bin
	install -c -m 444 lscpu.1 ${PREFIX}/man/man1

clean:
	rm -f lscpu
