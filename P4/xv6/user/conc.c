#include "conc.h"
#include "types.h"
#include "user.h"
#include "fcntl.h"

#define PGSIZE 4096

typedef struct threadstack
{
  int pid;
  void* space;
  struct threadstack* next;
} threadstack;

threadstack* first = NULL;
threadstack* new  = NULL;
threadstack* last = NULL;

int thread_create(void (*start_routine)(void*), void* arg)
{
  int pid = -1;
  void* pages = malloc(2*PGSIZE);
  void* tsp = pages; // page align this version
  int gap = (int)pages % PGSIZE;
  
  if(gap != 0) // not page aligned
  {
    tsp += (PGSIZE-gap);
  }
  else // page aligned already
    tsp = pages;
  
  pid = clone(start_routine, arg, tsp);
  
  if (pid != -1)
  {
    new = malloc(sizeof(threadstack));
    new->pid = pid;
    new->space = pages; // both pages
    new->next = NULL; // NULL terminate linked list
    
    if(first == NULL) // first thread created
    {
      first = new;
      last = new;
    }
    else // append
    {
      last->next = new;
      last = new;
    }
  }
  
  return pid;
}

int thread_join(int pid)
{
  int threadExists = 0;
  int retpid = -1;
  threadstack* tmp;
  threadstack* preFound;

  if(first == NULL)
    return -1; // nothing to wait for!
  else
    tmp = first; // for iterations
    
  do // search for pid in our known list
  {
    if(tmp->pid == pid)
    {
      threadExists = 1;
      break;
    }
    
    // i++
    preFound = tmp;
    tmp = tmp->next;
  }
  while(tmp->next != NULL);
  
  if(threadExists || pid == -1)
  {
    retpid = join(pid);
    
    if(retpid == -1)
      printf(1, "unexpected retpid from join");
    
    // free the memory after the wait
    free(tmp->space);
    
    // maintain meta list
    if(preFound != NULL) // list not empty
    {
      if(tmp->next == NULL)
        last = preFound;
        
      preFound->next = tmp->next;
    }
    else // linked list is empty again
    {
      first = NULL;
      last = NULL;
    }
    
    free(tmp);
  }
  
  return retpid; 
}

void lock_init(lock_t* lock)
{

}

void lock_acquire(lock_t* lock)
{

}

void lock_release(lock_t* lock)
{

}

void cv_wait(cond_t*, lock_t*)
{

}

void cv_signal(cond_t*)
{

}
