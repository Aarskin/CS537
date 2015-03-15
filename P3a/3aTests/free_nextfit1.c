/* a simple next fit allocation followed by a free */
#include <assert.h>
#include <stdlib.h>
#include "mymem.h"

int main() {
   assert(Mem_Init(4096,64) != NULL);
   void* ptr = Mem_Alloc(32);
   assert(ptr != NULL);
   assert(Mem_Free(ptr) == 0);
   exit(0);
}
