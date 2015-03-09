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
void* slabSegFault;			// Address of the byte after the slab segment
void* nextSegFault;			// Address of the byte after the next segment

void* Mem_Init(int sizeOfRegion, int slabSize)
{
	void* addr = NULL;	// Starting address of newly mapped mem
	//int trueSize = sizeOfRegion + sizeof(struct FreeHeader); // Count meta
	
	if(!initialized) // Only do this stuff once
	{
		while(sizeOfRegion % 16 != 0) // 16 bit alignment
			sizeOfRegion++;
		
		slabSegSize = sizeOfRegion/4; // 1/4 of the space
		nextSegSize = sizeOfRegion - slabSegSize; // 3/4 of the space
		specialSize = slabSize;

		// The allocation
		addr = mmap(NULL, sizeOfRegion, PROT_READ | PROT_WRITE, 
						MAP_ANON | MAP_PRIVATE, -1, 0);
			
		assert(addr != MAP_FAILED); // Bail on error (for now?)
		initialized = true;
				
		// Slab segment init
		int headerSize = sizeof(struct FreeHeader);
		slabHead = (struct FreeHeader*)addr; // addr returned by mmap
		void* finalSlabStart = ((void*)slabHead) + slabSegSize - headerSize;
		
		struct FreeHeader* tmp = slabHead; // i = 0
		void* nextSlab;	
		
		while((void*)tmp <= finalSlabStart)
		{
			nextSlab = ((void*)tmp)+slabSize; // Address of nextSlab
		
			//tmp->length = slabSize; // Redundant (special size)
			
			if(nextSlab <= finalSlabStart)
				tmp->next	= nextSlab; // Room for more
			else
				tmp->next = NULL; // This is the last slab
			
			tmp = (struct FreeHeader*)nextSlab;	
		}
		
		slabSegFault = ((void*)slabHead) + slabSegSize;
				
		// Next fit segment init
		nextHead			= (struct FreeHeader*)(addr + slabSegSize);
		nextHead->length	= nextSegSize - sizeof(struct FreeHeader);
		nextHead->next		= NULL;
		
		nextSegFault = ((void*)nextHead) + nextSegSize;
	}
	
	return addr; // Null pointer if initialized already!
}

void* Mem_Alloc(int size)
{
	// Spin size up to a multiple of 16 (modulo thinking was hurting)
	while(size % 16 != 0)
		size++;

	struct AllocatedHeader* allocd = NULL;
	
	if(size == specialSize) // Try slab allocation
		allocd = SlabAlloc(size);
	
	// If allocd is still NULL here, we either didn't try SlabAlloc,
	// or it failed and we should try NextAlloc. If it's not NULL,
	// assume SlabAlloc was tried and did work;.
	if(allocd == NULL)
		allocd = NextAlloc(size);		
	
	if(allocd != NULL) // Move allocd to the actually free byte in memory
		allocd = ((void*)allocd) + sizeof (*allocd);
		
	return allocd; // NULL if both fail		
}

struct AllocatedHeader* SlabAlloc(int size)
{
	struct AllocatedHeader* allocd = NULL;
	
	if(slabHead->length < size)
	{
		return allocd; // NO SPACE 4 U
	}
	else if(slabHead->length >= size)
	{
		// Give the allocated header the byte just after the slabHead
		allocd = (struct AllocatedHeader*)(((void*)slabHead)
					+sizeof(struct FreeHeader));
					
		allocd->length = size;
		
		return 0; // REmove
		/*
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
		*/
	}
	else
	{
		printf("HOW?");
		exit(0);
	}
}

// Size will be a multiple of 16
struct AllocatedHeader* NextAlloc(int size)
{
	struct AllocatedHeader* allocd = NULL;
	struct FreeHeader* check = nextHead; // Temp, for iteration
	int trueSize = size + sizeof(struct AllocatedHeader);
	
	do
	{	// There is room in this block (even when considering bookkeeping)
		if(check->length >= trueSize) 
		{
			// Populate newly allocated meta (at the start of the block)
			allocd		= ((void*)check) + sizeof(check);
			allocd->length = size;
			allocd->magic	= (void*)MAGIC;
		
			// Calculating...
			void* nextFreeByte = ((void*)allocd) + sizeof(*allocd) + size;
			int remainingLength = check->length - sizeof(*allocd) - trueSize;
		
			// Checks if there are at least 16 bytes available for the
			// storage of another FreeHeader. This header could potentially
			// have 0 length (redundancy/performance hit)
			//if(nextFreeByte + sizeof(FreeHeader) <= nextSegFault)
			
			if(remainingLength >= 0)
			{
				// Create new FreeHeader 
				struct FreeHeader* newBlock = nextFreeByte;
				newBlock->length = remainingLength;
				
				// Inserting newBlock into the freelist chain. If check was 
				// the last header, newBlock will be the new last header. 
				// Otherwise, newBlock will point wherever check did.
				newBlock->next = check->next;			
			}
				
			// Update outdated information
			check->length = 0; // This block is empty now
			
			if(remainingLength >= 0)
				check->next = nextFreeByte;
			else
				check->next = NULL;
			
			break; // Stop looping, we are done.
		}
		else check = check->next; // Move to the next FreeHeader	
	} while (check != NULL); 	 // Until we reach the end of the freelist
		
	return allocd; // NULL on failure
}

int Mem_Free(void *ptr)
{
	// Validate ptr
	if(ptr == NULL)
		return 0; // Do nothing, not even err
	else if(!ValidPointer(ptr))
	{
		fprintf(stderr, "SEGFAULT\n");
		return -1;
	}
	else // Valid ptr
	{
		// This *should be* the ptr's AllocatedHeader
		struct AllocatedHeader* allocd = ptr - sizeof(struct AllocatedHeader);		
		assert(allocd->magic == (void*)MAGIC); // A real AllocatedHeader		
		int freedSpace = allocd->length + sizeof(*allocd);
		
		int check = NextCoalesce(allocd, freedSpace);
		assert(check == 0);
	}
	
	return 0;
}

int NextCoalesce(void* ptr, int freeBytes)
{
	struct FreeHeader* tmp = nextHead; // Start at the beginning
	struct FreeHeader* lastBefore; // Last FreeHeader* before new mem
	struct FreeHeader* firstAfter; // First FreeHeader* after new mem
	void* postTmp; // The addr immediately after the current FreeHeader space
	bool passed = false; // Flag for passing the freed space in memory
	
	void* startFree = ptr; // Starting address of newly freed space
	void* postFree = ptr + freeBytes; // First still occupied address
	
	while(tmp != NULL) // Walk along the freelist
	{
		postTmp = ((void*)tmp)+sizeof(*tmp)+tmp->length;
		
		if(postTmp == startFree) // Contiguous! (tmp before ptr)
		{
			return 0;
		}
		
		if(((void*)tmp) == postFree) // Contiguous! (ptr before tmp)
		{
			return 0;
		}
		
		if(((void*)tmp) > postFree && !passed) // First FreeHeader after space
		{
			firstAfter = tmp;
			passed = true;
		}
		
		if(!passed) // Still before the freed space in memory
		{
			lastBefore = tmp;		
		}
		
		// Increment to next FreeHeader
		tmp = tmp->next;
	}
	
	return 0;
}

bool ValidPointer(void* ptr)
{
	void* mmapEnd = ((void*)slabHead) + slabSegSize + nextSegSize;
	
	if(ptr < ((void*)slabHead) || ptr >= mmapEnd)
		return false;
	else
		return true;
}

void Mem_Dump()
{	
	Dump(slabHead, "SLAB");
	Dump(nextHead, "NEXT FIT");

	return;
}

// Dump a segment freelist given its head ptr
int Dump(struct FreeHeader* head, char* name)
{
	int i = 1;
	struct FreeHeader* tmp = head;
	
	// Output the *actually* free space
	//void* firstByte = ((void*)head)+sizeof(struct FreeHeader);
	
	printf("%s HEAD\n", name);
	printf("--------------------------\n");
	printf("ADDR: %p\n", tmp);
	printf("LENGTH: %d\n", tmp->length);
	//printf("FIRST BYTE: %p\n", firstByte);
	printf("NEXT HEADER: %p\n", tmp->next);
	printf("\n");
	
	while(tmp->next != NULL)
	{
		tmp = (struct FreeHeader*)tmp->next;
		
		printf("Space %d\n", i);
		printf("--------------------------\n");
		printf("ADDR: %p\n", tmp);
		printf("LENGTH: %d\n", tmp->length);
		//printf("FIRST BYTE: %p\n", firstByte);
		printf("NEXT HEADER: %p\n", tmp->next);
		printf("\n");
		
		i++;
	}
	
	return 0;
}
