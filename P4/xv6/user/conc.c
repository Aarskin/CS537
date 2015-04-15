#include "conc.h"
#include "types.h"
#include "user.h"
#include "fcntl.h"

#define PGSIZE 4096

int thread_create(void (*start_routine)(void*), void* arg)
{
  int pid = -1;
  void* tsp = malloc(2*PGSIZE);
  int gap = (int)tsp % PGSIZE;
  
  if(gap != 0) // not page aligned
  {
    tsp += (PGSIZE-gap);
  }
  
  pid = clone(start_routine, arg, tsp);
  
  return pid;
}
