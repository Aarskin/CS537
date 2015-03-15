/* verify 16 byte alignment for next fit allocator */
#include <assert.h>
#include <stdlib.h>
#include "mymem.h"
#include <stdio.h>
#include <stdint.h>

int main() {
   char *ptr;
   ptr = (char *)Mem_Init(4096, 64);
   assert(ptr != NULL);

   char* nfPtr = (char *)Mem_Alloc(1);
   assert(nfPtr != NULL);
   assert(nfPtr-ptr-sizeof(struct AllocatedHeader) >= 1024);
   assert(((uintptr_t)nfPtr)%16  == 0);

   nfPtr = (char *)Mem_Alloc(3);
   assert(nfPtr != NULL);
   assert(nfPtr-ptr-sizeof(struct AllocatedHeader) >= 1024);
   assert(((uintptr_t)nfPtr)%16 == 0);

   exit(0);
}
