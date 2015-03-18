#include <sys/mman.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <assert.h>
#include <pthread.h>
#include <time.h>
#include <string.h>
#include "mymem.h"
#include "else.h"

int specialSize = -1;		// Slab size
bool initialized = false;	// Flag for Mem_Init

int slabSegSize;			// Size of slab segment in bytes
int nextSegSize;			// Size of next segment in bytes
struct FreeHeader* slabHead;	// Slab allocator freelist HEAD
struct FreeHeader* nextHead;	// Nextfit allocator freelist HEAD
struct FreeHeader* nextStart;	// For use by nextFit algorithm
void* slabSegFault;			// Address of the byte after the slab segment
void* nextSegFault;			// Address of the byte after the next segment

// Head pointers can move, beware!
void* sHead;	// Static ptr to head of slab segment
void* nHead;	// Static ptr to head of next segment

pthread_mutex_t sLock = PTHREAD_MUTEX_INITIALIZER;	// Slab seg lock
pthread_mutex_t nLock = PTHREAD_MUTEX_INITIALIZER;	// Next seg lock
pthread_mutex_t gLock = PTHREAD_MUTEX_INITIALIZER;	// Huge fat lock

///// DEBUG ONLY
struct FreeHeader* lastSlab;

struct FreeHeader* getLastSlab()
{
	return lastSlab;
}
////////////////

void* Mem_Init(int sizeOfRegion, int slabSize)
{
	void* addr = NULL;	// Starting address of newly mapped mem
	//int trueSize = sizeOfRegion + sizeof(struct FreeHeader); // Count meta
	
	Pthread_mutex_lock(&gLock);
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
			
		if(addr == MAP_FAILED)
			addr = NULL; // Something is wrong
		else
		{
			initialized = true;
				
			// Slab segment init
			//int headerSize = sizeof(struct FreeHeader);
			slabHead = (struct FreeHeader*)addr; // addr returned by mmap
			slabSegFault = ((void*)slabHead) + slabSegSize;
			void* finalSlabStart = ((void*)slabHead) + slabSegSize - specialSize;
		
			struct FreeHeader* tmp = slabHead; // i = 0
			void* nextSlab;	
		
			while((void*)tmp <= finalSlabStart)
			{
				nextSlab = ((void*)tmp)+slabSize; // Address of nextSlab
		
				//tmp->length = slabSize; // Redundant (special size)
			
				if((void*)tmp < finalSlabStart)
					tmp->next	= nextSlab; // Room for more
				else
				{
					tmp->next = NULL; // This is the last slab
				
					///// DEBUG USE ONLY
					lastSlab = tmp;
					////////////////////
				}
			
				tmp = (struct FreeHeader*)nextSlab; // Advance
			}
				
			// Next fit segment init
			nextHead			= (struct FreeHeader*)(addr + slabSegSize);
			nextHead->length	= nextSegSize - sizeof(struct FreeHeader);
			nextHead->next		= NULL;
		
			nextStart = nextHead; // First call to alloc starts here
		
			nextSegFault = ((void*)nextHead) + nextSegSize;
		
			/*
			// Initialize Locks!!
			sLock = PTHREAD_MUTEX_INITIALIZER;
			nLock = PTHREAD_MUTEX_INITIALIZER;
			*/
		
			// Head pointers can move, beware!
			sHead= slabHead;
			nHead= nextHead;
		}
	}
	Pthread_mutex_unlock(&gLock);
	
	return addr; // Null pointer if initialized already (or error)!
}

void* Mem_Alloc(int size)
{
	Pthread_mutex_lock(&gLock);
	struct AllocatedHeader* allocd = NULL;
	
	if(size == specialSize) // Try slab allocation
	{
		//Pthread_mutex_lock(&sLock);		
		allocd = SlabAlloc(size);
		//Pthread_mutex_unlock(&sLock);
	}
		
	// Spin size up to a multiple of 16 (modulo thinking was hurting)
	while(size % 16 != 0)
		size++;
	
	// If allocd is still NULL here, we either didn't try SlabAlloc,
	// or it failed and we should try NextAlloc. If it's not NULL,
	// assume SlabAlloc was tried succesfully
	if(allocd == NULL)
	{
		//Pthread_mutex_lock(&nLock);
		allocd = NextAlloc(size);
		//Pthread_mutex_unlock(&nLock);
		
		if(allocd != NULL) // Move allocd to the actually free byte in memory
			allocd = ((void*)allocd) + sizeof (*allocd);
	}
	Pthread_mutex_unlock(&gLock);
		
	return allocd; // NULL if both fail		
}

struct AllocatedHeader* SlabAlloc(int size)
{
	struct AllocatedHeader* allocd = NULL;	// Not filled, just an address
	
	if(size != specialSize) return allocd;	// The impossible case
	if(slabHead == NULL) return allocd;	// Nothing here

	// Give the allocated header the byte just after the slabHead and advance
	// the slabHead to the next slab
	allocd = (void*)slabHead;
	slabHead = slabHead->next;
	
	// Wipe it clean
	memset(allocd, 0, specialSize);
	
	// Dont give out nextfit slabs!
	if((void*)slabHead >= slabSegFault)
		slabHead = NULL; // No slabs left
	
	return allocd;
}

// Size will be a multiple of 16
struct AllocatedHeader* NextAlloc(int size)
{
	struct AllocatedHeader* allocd = NULL;
	struct FreeHeader* check = nextStart; // Temp, for iteration
	struct FreeHeader* prev = NULL; // The most recently failed FreeHeader
	
	if(nextHead == NULL) return allocd; // Is memory full?
	
	do // Check every FreeHeader starting with nextStart
	{	
		if(check->length >= size)
		{
			allocd = ((void*)check); // Replaces check FreeHeader
			
			// Calculating...
			void* nextFreeByte = ((void*)allocd) + sizeof(*allocd) + size;
			int remainingLength = check->length - size;
			
			if(remainingLength > 0)
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
				if(check == nextStart /*defensive*/ && prev == NULL)
				{
					nextHead = newBlock;
				}
				// Prev was pointing to the block we just replaced with an
				// AllocatedHeader (prev d.n.e. if nextHead). Point it to
				// the newBlock instead.
				else
				{
					prev->next = newBlock;
				}
				
				// Either way, start at the new block on the next alloc
				nextStart = newBlock;
			}
			else // we filled this free block
			{
				// Loop around to the nextHEAD->next, make sure to set 
				// the nextHead to NULL if it happened to be pointing to 
				// nextStart (which is being released)
				if(nextStart->next == NULL)
				{
					if(nextHead->next == nextStart)
						nextHead->next = NULL; // New end
				
					// Will remain NULL if mem filled,
					// loop around otherwise
					nextStart = nextHead;			
				}
				else // simply advance the nextStart HEAD
				{
					// If we happened to be pointed at nextStart (which is
					// disappearing) make sure we follow it (to the next
					// available free block)
					if(nextHead->next == nextStart)
						nextHead->next = nextStart->next;
						
					nextStart = nextStart->next;
				}
			}
			
			// Overwrite check after you are done with it!
			allocd->length = size;
			allocd->magic	= (void*)MAGIC;
						
			if(allocd->length == specialSize)
			{
				void* start = ((void*)allocd)+sizeof(*allocd);
				memset(start, 0, specialSize);
			/*
				// Wipe the memory clean, starting here
				int* start = ((void*)allocd+sizeof(*allocd));
			 
				int i;
				for(i = 0; i < 16; i++)
				{
					*(start+i) = 0;
				}
				*/
			}
			
			break; // Stop looping, there was room in check.
		}
		else // There was no room in check
		{
			prev = check;
			check = check->next; // Move to the next FreeHeader
			
			// Loop back around (will terminate next pass if we started at
			// the nextHead, as nextHead and nextStart would be equivalent)
			if(check == NULL)
			{
				check = nextHead;
			}	
		}
	} while (check != nextStart); // Until we loop around
		
	return allocd; // NULL on failure
}

int Mem_Free(void *ptr)
{
	int ret = 0; // Assume success
	
	Pthread_mutex_lock(&gLock);
	seg_t seg = PointerCheck(ptr);
	
	if(ptr == NULL) 
		ret = 0; // Do nothing, not even err
	else if(seg == FAULT)
	{
		fprintf(stderr, "SEGFAULT\n");
		ret = -1; // Redundant
	}
	else if(seg == NEXT) // Potential nextSeg pointer
	{
		//Pthread_mutex_lock(&nLock);
		// This *should be* the ptr's AllocatedHeader
		struct AllocatedHeader* allocd = ptr - sizeof(struct AllocatedHeader);
		// Fail if this is not an allocatedHeader
		if(!(allocd->magic == (void*)MAGIC))
			ret = -1; // Can't free it! 
		else
		{
			allocd->magic = 0; // Not allocated anymore
			int freedSpace = allocd->length + sizeof(*allocd);
		
			// Add this chunk of memory back into the freelist chain
			int check = NextCoalesce(allocd, freedSpace);
			assert(check == 0);
			//Pthread_mutex_unlock(&nLock); 
		}
	}
	else if(seg == SLAB) // Potential slabSeg pointer
	{
		//Pthread_mutex_lock(&sLock);
		// slabHead can (WILL) move. Beware! (sHead can't)
		void* sHead = slabSegFault - slabSegSize;
		int offset = ptr - ((void*)sHead);
		int check = 0;
		
		// The address of the pointer passed in should be offset from the 
		// sHead pointer by an even multiple of specialSize (slab starts)
		if(offset % specialSize == 0)
		{
			check = SlabCoalesce(ptr); // Maintain the free chain		
			assert(check == 0);
			ret = 0;
		}
		else
		{
			ret = -1;
		}
		
		//Pthread_mutex_unlock(&sLock);
	}
	else
	{
		// The impossible case
		ret = -1;
	}
	Pthread_mutex_unlock(&gLock);

	return ret;
}

void Pthread_mutex_lock(pthread_mutex_t* lock)
{
	int check = pthread_mutex_lock(lock);
	assert(check == 0);
}

void Pthread_mutex_unlock(pthread_mutex_t* lock)
{
	int check = pthread_mutex_unlock(lock);
	assert(check == 0);
}

seg_t PointerCheck(void* ptr)
{	
	int ret = FAULT; // Default to failing
	
	if(ptr < sHead || ptr >= nextSegFault)
		ret = FAULT;
	else if (ptr >= sHead && ptr < slabSegFault)
		ret = SLAB;
	else if (ptr >= nHead && ptr < nextSegFault)
		ret = NEXT;
	else
		assert(false); // Shouldn't be possible
		
	return ret;
}

// We know ptr to be a valid slab pointer
int SlabCoalesce(void* ptr)
{
	struct FreeHeader* freedSlab	= (struct FreeHeader*)ptr;
	struct FreeHeader* tmp		= slabHead; // For walking
	
	if(slabHead == NULL) // Segment was full before the free was made
	{
		slabHead = (struct FreeHeader*)ptr;
		slabHead->next = NULL; // This is the only one now
		
		return 0; // Nothing else to possibly link to
	}
	
	// Check if the slab being freed is above anything currently free. We need
	// to make sure that slabHead is always the first free slab
	if(((void*)slabHead) >= ptr)
	{
		struct FreeHeader* oldHead = slabHead;
		slabHead = (struct FreeHeader*)ptr;
		slabHead->next = oldHead;
		
		return 0; // Only link necessary
	}
	
	// slabHead is the first free slab, walk down the list
	while(tmp != NULL)
	{
		// Is freedSlab already part of the freelist?
		if(((void*)tmp->next) == ptr)
		{
			break; // Already in list (nothing to do)
		}		
		
		// If the tmp->next skips over freedSlab, we know whats up
		if(((void*)tmp->next) > ptr)
		{
			freedSlab->next = tmp->next;
			tmp->next = freedSlab;
			
			break; // Boom, done
		}		
		
		// We looked at every. single. header. and never passed the freedSlab?
		// It MUST be after the end of our list (or we wouldn't be here)
		if(tmp->next == NULL)
		{
			tmp->next = freedSlab;
			freedSlab->next = NULL;
			
			break; // Boom, done
		}
		
		tmp = tmp->next; // Advance to next known FreeHeader
	}
	
	return 0;
}

// freeBytes already accounts for the AllocatedHeader that was freed
int NextCoalesce(void* ptr, int freeBytes)
{
	// Make sure there is something to do, if nextHead is NULL, ptr is new head
	if(nextHead == NULL)
	{
		assert(nextStart == NULL); // D###
		
		nextHead			= (struct FreeHeader*)ptr;
		nextHead->length	= freeBytes - sizeof(*nextHead);
		nextHead->next		= NULL; // All alone again
		
		nextStart = nextHead; // Next fit also needs to start here
		
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
			
			nextStart = nextHead; // Need to follow the head
		}
		else // They are NOT contiguous, point to your old self
		{
			nextHead->length = freeBytes - sizeof(*nextHead);
			nextHead->next = oldHead;
		}		 
		
		return 0; // No more contiguity possible
	}

	while(tmp != NULL) // Walk along the freelist, tmp == nextHead
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
				
				// If the fitCheck block is where the nextFit was planning
				// on starting prior to this coalition (nextStart), then 
				// after this coalition, nextStart would be pointing to the
				// middle of a free region, rather than it's head! No good,
				// we need it to point to the start of the region (tmp)
				if(fitCheck == nextStart)					
					nextStart = tmp;
			}			
			
			return 0; // We're done. Can't possibly be more coalesced
		}
		
		if(((void*)tmp) == postFree) // Contiguous! (ptr before tmp)
		{
			newFree = (struct FreeHeader*)ptr; // Replace old AllocatedHeader
			newFree->length	= freeBytes + sizeof(*tmp) + tmp->length;
			newFree->next		= tmp->next;
			
			// If tmp is nextStart, we need to rewind nextStart to the head 
			// of this newly free region for nextAlloc to function correctly
			if(tmp == nextStart)
				nextStart = newFree;
			
			// No need to perfect fit check here, because if the previous 
			// FreeHeader was contiguous, it would have triggered 'if #1' on
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
	
	// This could happen when:
	// A) We found the true lastBefore and firstAfter FreeHeaders, neither were
	//		contiguous, so we need to link the newly freed space to both (if#3)
	if(lastBefore != NULL && firstAfter != NULL)
	{
		// Link old to new
		lastBefore->next	= newFree; // Length is unchanged
		
		// Link new to old
		newFree->length	= freeBytes - sizeof(*newFree);
		newFree->next		= firstAfter;
	}
	// B) The newly freed space is after the last currently available free 
	// 		space (non-contiguously, otherwise if #1 would have run)
	else if(firstAfter == NULL)
	{
		// Link old to new
		lastBefore->next	= newFree; // Length is unchanged
		
		// Initialize new space
		newFree->length	= freeBytes - sizeof(*newFree);
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
		fprintf(stderr, "\n%s Segment Filled\n", name);
		return 0;
	}
	
	fprintf(stderr, "%s HEAD\n", name);
	fprintf(stderr, "---------------------\n");
	fprintf(stderr, "ADDR: %p\n", tmp);
	fprintf(stderr, "LENGTH: %d\n", tmp->length);
	//printf("FIRST BYTE: %p\n", firstByte);
	fprintf(stderr, "NEXT: %p\n", tmp->next);
	fprintf(stderr, "\n");
	
	while(tmp->next != NULL)
	{
		tmp = (struct FreeHeader*)tmp->next;
		
		fprintf(stderr, "Space %d\n", i);
		fprintf(stderr, "---------------------\n");
		fprintf(stderr, "ADDR: %p\n", tmp);
		fprintf(stderr, "LENGTH: %d\n", tmp->length);
		//printf("FIRST BYTE: %p\n", firstByte);
		fprintf(stderr, "NEXT: %p\n", tmp->next);
		fprintf(stderr, "\n");
		
		i++;
	}
	
	return 0;
}
