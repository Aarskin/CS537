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
			allocd		= ((void*)check) + sizeof(*check);
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
				// the last header, newBlock will be the new last header. c
				// Otherwise, newBlock will point wherever check did.
				newBlock->next = check->next;			
			}
				
			// Update outdated information
			check->length = 0; // This block is empty now
			
			if(remainingLength >= 0)
				check->next = nextFreeByte; // nextFreeByte == newBlock
			else
				check->next = NULL;
			
			break; // Stop looping, there was room in check.
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
		assert(allocd->magic == (void*)MAGIC); // An active AllocatedHeader
		allocd->magic = 0;	// Not anymore	
		int freedSpace = allocd->length + sizeof(*allocd);
		
		// Add this chunk of memory back into the freelist chain
		int check = NextCoalesce(allocd, freedSpace);
		assert(check == 0);
	}
	
	return 0;
}

// freeBytes already accounts for the AllocatedHeader being freed
int NextCoalesce(void* ptr, int freeBytes)
{
	struct FreeHeader* tmp = nextHead; // Start at the beginning
	struct FreeHeader* lastBefore = NULL; // Last FreeHeader* before new mem
	struct FreeHeader* firstAfter = NULL; // First FreeHeader* after new mem
	void* postTmp; // The addr immediately after the current FreeHeader space
	bool passed = false; // Flag for passing the freed space in memory
	
	void* startFree = ptr; // Starting address of newly freed space
	void* postFree = ptr + freeBytes; // First still occupied address
	
	struct FreeHeader* newFree = NULL; // Used when non-contiguous
	struct FreeHeader* fitCheck = NULL; // Used in the perfect fit case
	
	while(tmp != NULL) // Walk along the freelist
	{
		postTmp = ((void*)tmp)+sizeof(*tmp)+tmp->length;
		
		if(postTmp == startFree) // Contiguous! (tmp before ptr)
		{
			// Update tmp's length by adding the freeBytes that are now
			// appended to it
			tmp->length += freeBytes;
			
			// Check if the newly freed space is ALSO continguous with the
			// FreeHeader after it (perfect fit case)
			fitCheck = tmp->next;
			
			if(((void*)fitCheck) == postFree)
			{
				// Add that free space to this one
				tmp->length += fitCheck->length + sizeof(*fitCheck);
				tmp->next = fitCheck->next; // Update linkage
			}			
			
			return 0; // We're done. Can't possibly be more coalesced
		}
		
		if(((void*)tmp) == postFree) // Contiguous! (ptr before tmp)
		{
			newFree = (struct FreeHeader*)ptr; // Replace old AllocatedHeader
			newFree->length	= freeBytes + sizeof(*tmp) + tmp->length;
			newFree->next		= tmp->next;
			
			// No need to perfect fit check here, because if the previous 
			// FreeHeader was contiguous, it would have triggered if #1 on
			// the last pass of the loop and this would never have run.
			return 0;
		}
		
		// First FreeHeader after newly freed space. It is not contiguous with
		// the freed space (otherwise an above if statement would run)
		if(((void*)tmp) > postFree && !passed) 
		{
			firstAfter = tmp;
			passed = true;
			
			// On the last pass of the loop, we tracked the FreeHeader that
			// was non-contiguously before the newly freed space. Now that we
			// have it's corrolary, there's no need to look at the other
			// FreeHeaders, just link these two up with the newly freed space
			break;
		}
		
		if(!passed) // Still before the newly freed space in memory
		{
			lastBefore = tmp;		
		}
		
		// Increment to the next FreeHeader
		tmp = tmp->next;
	}
	
	newFree = (struct FreeHeader*)ptr; // Replace old AllocatedHeader
	
	// Dropping out of the loop implies checking every FreeHeader for 
	// contiguity and finding none. This could happen when:
	// A) We found the true lastBefore and firstAfter FreeHeaders, neither were
	//		contiguous, so we need to link the newly freed space to both
	if(lastBefore != NULL && firstAfter != NULL)
	{
		// Link old to new
		lastBefore->next	= newFree; // Length is unchanged
		
		// Link new to old
		newFree->length	= freeBytes;
		newFree->next		= firstAfter;
	}
	// B) The newly freed space is after the last currently available free 
	// 		space (non-contiguously, otherwise if #1 would have run)
	else if(firstAfter == NULL)
	{
		// Link old to new
		lastBefore->next	= newFree; // Length is unchanged
		
		// Initialize new space
		newFree->length	= freeBytes; 
		newFree->next		= NULL; // New end of freelist
	}
	else
	{
		// Shouldn't happen
		fprintf(stderr, "Coalesce failed");
		return -1;
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
	//Dump(slabHead, "SLAB");
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
	
	printf("\n%s HEAD\n", name);
	printf("---------------------\n");
	printf("ADDR: %p\n", tmp);
	printf("LENGTH: %d\n", tmp->length);
	//printf("FIRST BYTE: %p\n", firstByte);
	printf("NEXT: %p\n", tmp->next);
	printf("\n");
	
	while(tmp->next != NULL)
	{
		tmp = (struct FreeHeader*)tmp->next;
		
		printf("Space %d\n", i);
		printf("---------------------\n");
		printf("ADDR: %p\n", tmp);
		printf("LENGTH: %d\n", tmp->length);
		//printf("FIRST BYTE: %p\n", firstByte);
		printf("NEXT: %p\n", tmp->next);
		printf("\n");
		
		i++;
	}
	
	return 0;
}
