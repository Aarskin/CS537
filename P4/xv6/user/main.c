#include "types.h"
#include "stat.h"
#include "user.h"
#include "thread.h"
#include "conc.h"

void dummy(void* arg);

int main(int argc, char *argv[])
{
  int pid;
  void* arg = (void*)5;
  //void* stack = (void*)35;  

  pid = thread_create(dummy, arg);
  join(pid);
  exit();
}

void dummy(void* arg)
{
  printf(1, "CLONE SUCCESS!");
  return;
}
