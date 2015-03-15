/* Fill the next fit region. Then free a contiguous space in the middle that should get
 * coalesced. Allocate to the coalesced space */
#include <assert.h>
#include <stdlib.h>
#include "mymem.h"
#include <stdio.h>

int main() {
   char *ptr = (char *)Mem_Init(4096, 64);
   assert(ptr != NULL);
   int i = 0;
   char *nfPtr = NULL, *nfPtr1 = NULL, *nfPtr2 = NULL;
   for(i=0; i<64; i++)
   {
	if(i == 13)
	{
		nfPtr1 = (char *)Mem_Alloc(32);
		assert(nfPtr1 != NULL);
	}
	else if(i == 14)
	{
		nfPtr2 = (char *)Mem_Alloc(32);
		assert(nfPtr2 != NULL);
	}
	else
	{
		nfPtr = (char *)Mem_Alloc(32);
		assert(nfPtr != NULL);
	}
   }
   assert(Mem_Alloc(32) == NULL);
   assert(Mem_Free(nfPtr1) == 0);
   assert(Mem_Free(nfPtr2) == 0);
   
   nfPtr = (char *)Mem_Alloc(55);
   assert(nfPtr != NULL);
   assert(nfPtr == nfPtr1);
   exit(0);
}
