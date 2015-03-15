/* call Mem_Init with size = 1 page */
#include "mymem.h"
#include <assert.h>
#include <stdlib.h>

int main() {
   char *ptr;
   ptr = (char *)Mem_Init(4096, 64);
   assert(ptr != NULL);
   ptr[4095] = 'c';
   exit(0);
}
