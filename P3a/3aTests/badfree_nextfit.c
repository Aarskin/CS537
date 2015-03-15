/* a simple next fit allocation followed by a bad free */

#include <assert.h>
#include <stdlib.h>
#include "mymem.h"

int main() {
   assert(Mem_Init(4096,64) != NULL);
   char* ptr = (char *)Mem_Alloc(32);
   assert(ptr != NULL);
   ptr += 1;
   assert(Mem_Free(ptr) == -1);
   exit(0);
}
