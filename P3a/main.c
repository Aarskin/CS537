#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include "mymem.h"
#include "else.h"

int main(int argc, char* argv[])
{
	//printf("Hello World!\n");
	
	void* addr = Mem_Init(256, 16);
	assert(addr != NULL);
	Mem_Dump();
	
	void* allocd = Mem_Alloc(10);
	assert(allocd != NULL);	
	Mem_Dump();
	
	exit(0);
}
