all:
	# Shared library compilation
	gcc -c -fpic mem.c -g -Wall -Werror
	gcc -shared -o libmem.so mem.o -g
	gcc -lmem -L. -o main -g main.c -Wall -Werror
	
clean:
	rm -f libmem.so main mem.o *~
