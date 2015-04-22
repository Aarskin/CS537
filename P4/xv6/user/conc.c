#include "conc.h"
#include "types.h"
#include "user.h"
#include "fcntl.h"
#include "x86.h"

#define PGSIZE 4096

// Linked list of allocated thread stacks
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
  
  if(pid != -1)
  {
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
  }
  
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
    
    free(tmp); // clean up
  }
  
  return retpid; 
}

void lock_init(lock_t* lock)
{
  lock->ticket = 0;
  lock->turn   = 0;
}

void lock_acquire(lock_t* lock)
{
  // Take a ticket
  int turn = fetchAndAdd(&lock->ticket, 1);
  // Wait until it's your turn (see lock_release)
  while(turn != lock->turn);
}

void lock_release(lock_t* lock)
{
  fetchAndAdd(&lock->turn, 1);
}

void cv_wait(cond_t* cond, lock_t* lock)
{
  struct pidBlock* block;
  struct pidBlock* tmp;
  
  // Create the block to place on the queue
  block = malloc(sizeof(struct pidBlock));
  block->pid = getpid();
  block->next = NULL;
  
  // For iteration
  tmp = cond->head;

  lock_acquire(lock);
  
  // Add to Queue
  if(cond->head == NULL) // only proc in queue
    cond->head = block;
  else // walk to the end of the chain and append
  {
    while(tmp->next != NULL)
      tmp = tmp->next;
      
    tmp->next = block;
  }
    
  cv_sleep(lock); // new syscall (need to write/ MUST RELEASE LOCK)
  
  lock_acquire(lock);
}

void cv_signal(cond_t* cond)
{
  struct pidBlock* newHead;
  
  if(cond->head != NULL) // there are processes waiting on this condition
  {
    // wake the first one    
    cv_wake(cond->head->pid); // new syscall (need to write)
    
    // Maintain queue
    newHead = cond->head->next;
    free(cond->head);
    cond->head = newHead;
  }
  // That's it, nothing waiting? no problem
}
