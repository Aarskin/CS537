#include <stdio.h>
#include <stdlib.h>
#include "mymem.h"

int main(int argc, char* argv[])
{
	printf("Hello World!\n");
	void* addr = Mem_Init(15, 8);	
	printf("Addr: %p\n", addr);
	exit(0);
}
