#include "types.h"
#include "stat.h"
#include "user.h"

int
main(int argc, char *argv[])
{
   // Step 1
   int* null = (int*)0x0;
   printf(1, "Address 0x0: %d\n", *null);
   exit();
}
