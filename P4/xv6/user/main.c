#include "types.h"
#include "stat.h"
#include "user.h"
#include "thread.h"

void dummy(void* arg);

int main(int argc, char *argv[])
{
  //int pid1, pid2;
  //int arg = 5;
  lock_t lock;
  cond_t cond;

/*
  pid1 = thread_create(dummy, (void*)&arg);
  pid2 = thread_create(dummy, (void*)&arg);
  printf(1, "MAIN CONTINUED. PID = %d\n", pid1);
  join(pid1);
  printf(1, "MAIN CONTINUED. PID = %d\n", pid2);
  join(pid2);
  
  lock_init(&lock);
  lock_acquire(&lock);
  lock_release(&lock);
*/  
  
  cv_wait(&cond, &lock);
  cv_wait(&cond, &lock);
  cv_signal(&cond);
  cv_wait(&cond, &lock);
  cv_signal(&cond);
  cv_signal(&cond);
    
  exit();
}

void dummy(void* arg)
{
  printf(1, "CLONE SUCCESS! ARG = %d\n", *(int*)arg);
  exit();
}
