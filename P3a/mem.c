#include <sys/mman.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <assert.h>
#include "mymem.h"
#include "else.h"

int specialSize = -1;		// Slab size
bool initialized = false;	// Flag for Mem_Init

int slabSegSize;			// Size of slab segment in bytes
int nextSegSize;			// Size of next segment in bytes
struct FreeHeader* slabHead;	// Slab allocator freelist HEAD
struct FreeHeader* nextHead;	// Nextfit allocator freelist HEAD
//void* firstFree;			// First truly free & mapped byte

void* Mem_Init(int sizeOfRegion, int slabSize)
{
	void* addr = NULL;	// Starting address of newly mapped mem
	//int trueSize = sizeOfRegion + sizeof(struct FreeHeader); // Count meta
	
	if(!initialized) // Only do this stuff once
	{
		// sizeOfRegion will be multiple of four (from spec)
		int slabSegSize = sizeOfRegion/4; // 1/4 of the space
		int nextSegSize = sizeOfRegion - slabSegSize; // 3/4 of the space
		specialSize = slabSize;

		// The allocation
		addr = mmap(NULL, sizeOfRegion, PROT_READ | PROT_WRITE, 
						MAP_ANON | MAP_PRIVATE, -1, 0);
			
		assert(addr != MAP_FAILED); // Bail on error (for now?)
		initialized = true;
		
		//DEBUG
		//printf("TRUEHEAD: %p\n", addr);
		//printf("SIZE: %lu\n", sizeof(struct FreeHeader));
		
		// Slab segment metadata
		slabHead			= (struct FreeHeader*)addr;
		slabHead->length	= slabSegSize - sizeof(struct FreeHeader);
		slabHead->next		= NULL; // Nothing in here yet
		
		// Next fit segment metadata
		nextHead			= (struct FreeHeader*)addr + slabSegSize;
		nextHead->length	= nextSegSize - sizeof(struct FreeHeader);
		nextHead->next		= NULL; // Nothing in here yet		
		
		//addr += sizeof(struct FreeHeader);
	}
	
	return addr; // Null pointer if initialized already!
}

void* Mem_Alloc(int size)
{
	struct AllocatedHeader* allocd = NULL;
	
	/*
	int trueSize = size + sizeof(struct AllocatedHeader); // Account for meta
	
	if(head->length < trueSize)
	{
		return allocd; // NO SPACE 4 U
	}
	else if(head->length >= trueSize)
	{
		// Initialize new AllocatedHeader
		allocd = (struct AllocatedHeader*)(((void*)head)+sizeof(struct FreeHeader));
		
		// Populate the header
		allocd->length = size; // Available to process
		allocd->magic	= (void*) MAGIC; // The non-SHA-SHA
		
		// Update freelist
		head->length -= trueSize; // Process space + Meta info
		
		if(head->length != trueSize) // Unless this request fits perfectly...
		{
			struct FreeHeader* newFreeBlock = (struct FreeHeader*)
				(((void*)head) + sizeof(struct FreeHeader) + trueSize);
			
			newFreeBlock->length = size; // Available to process
			newFreeBlock->next   = NULL; // NULL
		}
		
		allocd += sizeof(struct AllocatedHeader); // Advance ptr to free space
	}
	else
	{
		printf("HOW?");
		exit(0);
	}
	*/
	
	return allocd; // Success!
}

int Slab_Alloc()
{
	return 0;
}

int Next_Alloc()
{
	return 0;
}

int Mem_Free(void *ptr)
{
	return 0;
}

void Mem_Dump()
{	
	Dump(slabHead, "SLAB");
	Dump(nextHead, "NEXT FIT");
	
	/*
	int i = 1;
	struct FreeHeader* slabTmp = slabHead;
	struct FreeHeader* nextTmp = nextHead;
	
	void* firstByte = ((void*)head)+sizeof(struct FreeHeader);
	
	printf("HEAD\n");
	printf("--------------------------\n");
	printf("LENGTH: %d\n", tmp->length);
	printf("FIRST BYTE: %p\n", firstByte);
	printf("NEXT HEADER: %p\n", tmp->next);
	printf("\n");
	
	while(tmp->next != NULL)
	{
		tmp = (struct FreeHeader*)tmp->next;
		
		printf("Space %d\n", i);
		printf("--------------------------\n");
		printf("LENGTH: %d\n", tmp->length);
		printf("FIRST BYTE: %p\n", firstByte);
		printf("NEXT HEADER: %p\n", tmp->next);
		printf("\n");
		
		i++;
	}
	*/

	return;
}

// Dump a segment freelist given its head ptr
int Dump(struct FreeHeader* head, char* name)
{
	int i = 1;
	struct FreeHeader* tmp = head;
	
	// Output the *actually* free space
	void* firstByte = ((void*)head)+sizeof(struct FreeHeader);
	
	printf("%s HEAD\n", name);
	printf("--------------------------\n");
	printf("LENGTH: %d\n", tmp->length);
	printf("FIRST BYTE: %p\n", firstByte);
	printf("NEXT HEADER: %p\n", tmp->next);
	printf("\n");
	
	while(tmp->next != NULL)
	{
		tmp = (struct FreeHeader*)tmp->next;
		
		printf("Space %d\n", i);
		printf("--------------------------\n");
		printf("LENGTH: %d\n", tmp->length);
		printf("FIRST BYTE: %p\n", firstByte);
		printf("NEXT HEADER: %p\n", tmp->next);
		printf("\n");
		
		i++;
	}
	
	return 0;
}
