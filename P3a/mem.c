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
	struct FreeHeader* prev = NULL; // The most recently failed FreeHeader
	
	if(nextHead == NULL) return allocd; // Is memory full?
	
	do // Check every FreeHeader starting at the lowest Addressed one
	{	
		if(check->length >= size) 
		{
			allocd = ((void*)check); // Replaces check FreeHeader
			
			// Calculating...
			void* nextFreeByte = ((void*)allocd) + sizeof(*allocd) + size;
			int remainingLength = check->length - size;
			
			// Juuust enough room for a FreeHeader
			if(remainingLength >= 16)
			{
				// Create new FreeHeader 
				struct FreeHeader* newBlock = nextFreeByte;
				newBlock->length = remainingLength - sizeof(*newBlock);
				
				// Inserting newBlock into the freelist chain. If check was 
				// the last header, newBlock will be the new last header.
				// Otherwise, newBlock will point wherever check did.
				newBlock->next = check->next;
				
				// Check is either the nextHead or not:
				// If it is, we are 'advancing' the nextHead struct 
				// (where it will be 'rewinded' during coalition)
				if(check == nextHead /*defensive*/ && prev == NULL)
				{
					nextHead = newBlock;
				}
				// Prev was pointing to the block we just replaced with an
				// AllocatedHeader (no such block for nextHead). Point it to
				// the newBlock instead.
				else
				{
					prev->next = newBlock;
				}
			}
			else
			{
				nextHead = NULL; // Mapped memory is full!
			}
			
			// Overwrite check after you are done with it!
			allocd->length = size;
			allocd->magic	= (void*)MAGIC;
			
			break; // Stop looping, there was room in check.
		}
		else
		{
			prev = check;
			check = check->next; // Move to the next FreeHeader
		}
	} while (check != NULL); 	 // Until we reach the end of the freelist
		
	return allocd; // NULL on failure
}

int Mem_Free(void *ptr)
{
	seg_t seg = PointerCheck(ptr);
	
	if(ptr == NULL)
		return 0; // Do nothing, not even err
	else if(seg == FAULT)
	{
		fprintf(stderr, "SEGFAULT\n");
		return -1;
	}
	else if(seg == NEXT) // Valid nextSeg pointer
	{
		// This *should be* the ptr's AllocatedHeader
		struct AllocatedHeader* allocd = ptr - sizeof(struct AllocatedHeader);		
		assert(allocd->magic == (void*)MAGIC); // An active AllocatedHeader
		allocd->magic = 0; // Not anymore	
		int freedSpace = allocd->length + sizeof(*allocd);
		
		// Add this chunk of memory back into the freelist chain
		int check = NextCoalesce(allocd, freedSpace);
		assert(check == 0);
	}
	else if(seg == SLAB) // Valid slabSeg pointer
	{
		
	}
	else
	{
		printf("Unexpected Pointer: %p\n", ptr);
		return -1;
	}
	
	return 0;
}

seg_t PointerCheck(void* ptr)
{
	// Head pointers can move, beware!
	void* sHead	= slabSegFault - slabSegSize;
	void* sEnd	= slabSegFault;
	void* nHead	= nextSegFault - nextSegSize;
	void* nEnd	= nextSegFault;
	
	int ret = FAULT;
	
	if(ptr < sHead || ptr >= nEnd)
		ret = FAULT;
	else if (ptr >= sHead && ptr < sEnd)
		ret = SLAB;
	else if (ptr >= nHead && ptr < nEnd)
		ret = NEXT;
	else
		assert(false); // Shouldn't be possible
		
	return ret;
}

// freeBytes already accounts for the AllocatedHeader that was freed
int NextCoalesce(void* ptr, int freeBytes)
{
	// Make sure there is something to do, if nextHead is NULL, ptr is new head
	if(nextHead == NULL)
	{
		nextHead			= (struct FreeHeader*)ptr;
		nextHead->length	= freeBytes - sizeof(*nextHead);
		nextHead->next		= NULL; // All alone again
		
		return 0; // Nothing else to possibly coalesce with
	}	
	
	struct FreeHeader* tmp = nextHead; // Start at the beginning
	struct FreeHeader* lastBefore = NULL; // Last FreeHeader* before new mem
	struct FreeHeader* firstAfter = NULL; // First FreeHeader* after new mem
	void* postTmp; // The addr immediately after the current FreeHeader space
	bool passed = false; // Flag for passing the freed space in memory
	
	void* startFree = ptr; // Starting address of newly freed space
	void* postFree = ptr + freeBytes; // First still occupied address
	
	struct FreeHeader* newFree = NULL; // Used when non-contiguous
	struct FreeHeader* fitCheck = NULL; // Used in the perfect fit case
	
	// We're freeing something 'above' anything currently free, this region
	// becomes the new nextHead with only post contiguity to consider (if there
	// was a contiguous free region before this, it should already be the head)
	// It's only possible for this to be contiguous with the (now) old head
	if(((void*)nextHead) > ptr)
	{
		struct FreeHeader* oldHead = nextHead; // Save info
		
		nextHead = (struct FreeHeader*)ptr;
		
		if(postFree == ((void*)oldHead)) // They are contiguous
		{
			int newLength = freeBytes /*- sizeof(*nextHead)?*/ + oldHead->length;
			
			nextHead->length = newLength;
			nextHead->next = oldHead->next;
		}
		else // They are NOT contiguous, point to your old self
		{
			nextHead->length = freeBytes - sizeof(*nextHead);
			nextHead->next = oldHead;
		}
		
		return 0; // No more contiguity possible
	}
	
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
		fprintf(stderr, "Coalesce failed\n");
		return -1;
	}
	
	return 0;
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
	
	if(head == NULL) 
	{
		printf("\nNext Fit Segment Filled\n");
		return 0;
	}
	
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
