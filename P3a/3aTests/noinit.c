/* alloc before init */
#include <assert.h>
#include <stdlib.h>
#include "mymem.h"
#include <stdio.h>

int main() {
   char* slabPtr = (char *)Mem_Alloc(64);
   assert(slabPtr == NULL);
   exit(0);
}
