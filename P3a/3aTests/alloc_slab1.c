/* a simple 8 byte allocation from the slab */
#include <assert.h>
#include <stdlib.h>
#include "mymem.h"
#include <stdio.h>

int main() {
   char *ptr;
   ptr = (char *)Mem_Init(4096, 64);
   assert(ptr != 0);
   char* slabPtr = (char *)Mem_Alloc(64);
   assert(slabPtr != NULL);
   assert(slabPtr-ptr < 1024);
   exit(0);
}
