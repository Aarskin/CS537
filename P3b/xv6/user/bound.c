/* syscall argument checks (null page) */
#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h"

#undef NULL
#define NULL ((void*)0)

#define assert(x) if (x) {} else { \
   printf(1, "%s: %d ", __FILE__, __LINE__); \
   printf(1, "assert failed (%s)\n", # x); \
   printf(1, "TEST FAILED\n"); \
   exit(); \
}

int
main(int argc, char *argv[])
{
  char *arg;

  int fd = open("tmp", O_WRONLY|O_CREATE);
  assert(fd != -1);

  /* trying to read data at zero, should fail*/
  arg = (char*) 0x0;
  int check = write(fd, arg, 10);
  assert(check == -1);

  /* within null page */
  arg = (char*) 0x400;
  assert(write(fd, arg, 1024) == -1);

  /* spanning null page and code, because the length is 2 */
  arg = (char*) 0xfff;
  assert(write(fd, arg, 2) == -1);

  printf(1, "TEST PASSED\n");
  exit();
}
