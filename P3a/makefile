all:
	# Shared library compilation
	gcc -c -fpic mem.c -g -Wall -Werror
	gcc -shared -o libmem.so mem.o -g
	gcc -lmem -L. -o main -g main.c -Wall -Werror

thread:
	gcc -c -fpic mem.c -g -Wall -Werror
	gcc -shared -o libmem.so mem.o -g
	gcc -lmem -L. -o main -g threaded.c -Wall -Werror -lpthread

debug: 
	gcc -g -o main *.c -Wall -Werror # DEBUG ONLY
	
clean:
	rm -f libmem.so main mem.o *~
