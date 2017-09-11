all:
	gcc -g -O2 -Wall -o lscpu lscpu.c

clean:
	rm -f lscpu