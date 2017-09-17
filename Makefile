CC?=cc

all:
	${CC} -g -O2 -Wall -o lscpu lscpu.c

clean:
	rm -f lscpu
