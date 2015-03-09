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
	
	// Test
	Initialize(init, slabSize);
	Mem_Dump();
	SingleAlloc();
	Mem_Dump();
	
	// We win
	announce("ALL TESTS PASSED");	
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
	void* allocd = Mem_Alloc(160);
	assert(allocd != NULL);
	printf("Sucess!\n");
}

void announce(char* announcement)
{
	printf("\n%s\n\n", announcement);
}

