#include <sys/mman.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <assert.h>
#include "else.h"
#include "mymem.h"

int specialSize = -1;		// Slab size
bool initialized = false;	// Flag for Mem_Init
struct FreeHeader* head;		// Freelist HEAD
//void* firstFree;			// First truly free & mapped byte

void* Mem_Init(int sizeOfRegion, int slabSize)
{
	void* addr = NULL;	// Starting address of newly mapped mem
	int trueSize = sizeOfRegion + sizeof(struct FreeHeader); // Count meta
	
	if(!initialized) // Only do this stuff once
	{
		// Spec'd
		specialSize = slabSize;

		// The allocation
		addr = mmap(NULL, trueSize, PROT_READ | PROT_WRITE, 
						MAP_ANON | MAP_PRIVATE, -1, 0);
			
		assert(addr != MAP_FAILED); // Bail on error (for now?)
		initialized = true;
		
		//DEBUG
		//printf("TRUEHEAD: %p\n", addr);
		//printf("SIZE: %lu\n", sizeof(struct FreeHeader));
		
		// Use the same space to track freelist
		head			= (struct FreeHeader*)addr;
		head->length	= sizeOfRegion; // Available to process
		head->next	= NULL;
		
		addr += sizeof(struct FreeHeader);
	}
	
	return addr; // Null pointer when initialized already!
}

void* Mem_Alloc(int size)
{
	struct AllocatedHeader* allocd = NULL;
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
	
	return allocd; // Success!
}

int Mem_Free(void *ptr)
{
	return 0;
}

void Mem_Dump()
{
	int i = 1;
	struct FreeHeader* tmp = head;
	
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

	return;
}
