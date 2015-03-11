#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include "mymem.h"
#include "tests.h"

void* baseAddr = NULL;
void* allocd = NULL;

int main(int argc, char* argv[])
{
	int init = 256;
	int slabSize = 20;
	
	printf("\nInit: %d | Slab Size: %d\n\n", init, slabSize);
	
	// Test
	Initialize(init, slabSize);
	Mem_Dump();
	
	printf("Allocate Slab Seg");
	void* ptr = Mem_Alloc(20);
	assert(ptr != NULL);
	//Mem_Dump();
	
	printf("Free Slab Segment");
	int check = Mem_Free(ptr);
	assert(check == 0);
	//Mem_Dump();
	AllocAll();
	//Mem_Dump();
	FreeAll();
	//Mem_Dump();
	NextAllocAndFree(1, 256);
	////Mem_Dump();
	//Tinker();
	//Mem_Dump();
	
	// We win
	printf("\nALL TESTS PASSED\n\n");	
	exit(0);
}

void Initialize(int init, int slabSize)
{
	printf("Initialize...\n");
	baseAddr = Mem_Init(init, slabSize);
	assert(baseAddr != NULL);
	printf("Success!\n\n");
}

void AllocAll()
{
	printf("Allocating entire space...\n");	
	allocd = Mem_Alloc(176);
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

void NextAllocAndFree(int sizePer, int vSpace)
{
	printf("Fill and Free...\n");
	while(sizePer % 16 != 0)
		sizePer++; // Spin up to 16 bit alignment

	// True size that will be consumed from the space available. Only used to
	// calculate expected number of requests that will succeed.
	int trueRequestSize = sizePer + sizeof(struct AllocatedHeader);
	int nextSegSize	= (3 * (vSpace/4));
	int expectedRequests= nextSegSize / trueRequestSize;
	
	void* allocdPtrs[expectedRequests];
	void* failureExpected;
	
	printf("Expected Num Requests: %d\n", expectedRequests);
	printf("Allocating...\n");

	int i;
	for(i = 0; i < expectedRequests; i++)
	{
		allocdPtrs[i] = Mem_Alloc(sizePer);
		assert(allocdPtrs[i] != NULL);
	}
	
	failureExpected = Mem_Alloc(sizePer);
	assert(failureExpected == NULL);
	
	printf("Freeing...\n");
	
	int j = 0;
	for(j = 0; j < expectedRequests; j++)
	{
		int check = Mem_Free(allocdPtrs[j]);
		assert(check == 0);
	}
	printf("Success!\n\n");
}

void Tinker()
{
	printf("Tinkering...\n");
	void* allocdPtrs[4];
	
	// Fill it up
	int i;
	for(i = 0; i < 3; i++)
	{
		allocdPtrs[i] = Mem_Alloc(1);
		assert(allocdPtrs[i] != NULL);
	}

	int test = Mem_Free(allocdPtrs[2]);
	assert(test == 0);

	// Empty out of order
	int j;
	for(j = 1; j >= 0; j--)
	{
		int check = Mem_Free(allocdPtrs[j]);
		assert(check == 0);
	}
	
	printf("Success!\n\n");
}

