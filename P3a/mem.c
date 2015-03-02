#include <sys/mman.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <assert.h>
#include "else.h"

extern bool initialized;

void* Mem_Init(int sizeOfRegion, int slabSize)
{
	void* addr = NULL; // Starting address of newly mapped mem

	if(!initialized) // Only do this stuff once
	{
		specialSize = slabSize;
		//const char* map = "procMap";

		//int newFile = open("procMap", O_RDWR); // Open file for mapping
		//assert(newFile != -1); // Bail on error

		addr = mmap(NULL, sizeOfRegion, PROT_READ | PROT_WRITE, 
			MAP_ANON | MAP_PRIVATE, -1, 0); // Private to proc 
			
		assert(addr != MAP_FAILED); // Bail on error (for now?)	
	}
	
	return addr; // Null pointer when initialized already!
}

void* Mem_Alloc(int size)
{
	return 0; // Null pointer!
}

int Mem_Free(void *ptr)
{
	return 0;
}

void Mem_Dump()
{
	return;
}
