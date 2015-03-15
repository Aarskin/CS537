/* Fill the next fit region. The next allocation after the fill should fail */
#include <assert.h>
#include <stdlib.h>
#include "mymem.h"
#include <stdio.h>

int main() {
   char *ptr = (char *)Mem_Init(4096, 64);
   assert(ptr != NULL);
   int i = 0;
   char *nfPtr;
   for(i=0; i<64; i++)
   {
   	nfPtr = (char *)Mem_Alloc(32);
	assert(nfPtr != NULL);
	assert(nfPtr-ptr-sizeof(struct AllocatedHeader) >= 1024);
   }
   assert(Mem_Alloc(32) == NULL);
   exit(0);
}
