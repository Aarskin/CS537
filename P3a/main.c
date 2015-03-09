#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include "mymem.h"
#include "tests.h"

void* allocd;

int main(int argc, char* argv[])
{
	int init = 256;
	int slabSize = 16;
	
	printf("\nInit: %d | Slab Size: %d\n\n", init, slabSize);
	
	// Test
	Initialize(init, slabSize);
	//Mem_Dump();
	AllocAll();
	//Mem_Dump();
	FreeAll();
	
	// We win
	printf("ALL TESTS PASSED\n\n");	
	exit(0);
}

void Initialize(int init, int slabSize)
{
	printf("Initialize...\n");
	void* addr = Mem_Init(init, slabSize);
	assert(addr != NULL);
	printf("Success!\n\n");
}

void AllocAll()
{
	printf("Allocating entire space...\n");	
	allocd = Mem_Alloc(160);
	assert(allocd != NULL);
	printf("Success!\n\n");
}

void FreeAll()
{	
	printf("Freeing entire space...\n");	
	int ret = Mem_Free(allocd);
	assert(ret == 0);
	printf("Success!\n\n");
}

