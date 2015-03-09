#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include "mymem.h"
#include "tests.h"

int main(int argc, char* argv[])
{
	int init = 256;
	int slabSize = 16;
	
	printf("\nInit: %d | Slab Size: %d\n\n", init, slabSize);
	
	Initialize(init, slabSize);
	SingleAlloc();
	
	//announce("MEMORY INITIALIZED");
	printf("\nMEMORY INITIALIZED\n\n");
	Mem_Dump();
		
	void* allocd = Mem_Alloc(176);
	assert(allocd != NULL);
	
	//announce("10 BYTES ALLOCATED!");
	printf("\nMEMORY ALLOCATED\n\n");
	Mem_Dump();
	
	exit(0);
}

void Initialize(int init, int slabSize)
{
	announce("Initialize...");
	void* addr = Mem_Init(init, slabSize);
	assert(addr != NULL);
	printf("Success!\n");
}

void SingleAlloc()
{
	announce("Single Allocation...");	
	void* allocd = Mem_Alloc(176);
	assert(allocd != NULL);
	printf("Sucess!\n");
}

void announce(char* announcement)
{
	printf("\n%s\n", announcement);
}

