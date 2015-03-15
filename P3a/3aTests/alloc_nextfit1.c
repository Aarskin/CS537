/* a simple 8 byte allocation from the next fit allocator */
#include <assert.h>
#include <stdlib.h>
#include "mymem.h"
#include <stdio.h>

int main() {
   char *ptr;
   ptr = (char *)Mem_Init(4096, 64);
   assert(ptr != NULL);
   char* nfPtr = (char *)Mem_Alloc(32);
   assert(nfPtr != NULL);
   assert(nfPtr-ptr-sizeof(struct AllocatedHeader) >= 1024);
   exit(0);
}
