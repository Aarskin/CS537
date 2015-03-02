all: 
	gcc -c -fpic mem.c -Wall -Werror
	gcc -shared -o libmem.so mem.o
	gcc -lmem -L. -o main main.c -Wall -Werror -g
	
clean:
	rm -f libmem.so main mem.o *~
