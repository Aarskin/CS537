/* bad argument to Mem_Init */
#include <assert.h>
#include <stdlib.h>
#include "mymem.h"

int main() {
   assert(Mem_Init(0, 64) == NULL);
   assert(Mem_Init(-1, 64) == NULL); 
   assert(Mem_Init(4096, -64) == NULL); 
   exit(0);
}
