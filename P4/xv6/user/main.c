#include "types.h"
#include "stat.h"
#include "user.h"
#include "thread.h"
#include "conc.h"

void dummy(void* arg);

int main(int argc, char *argv[])
{
  int pid;
  int arg = 5;

  pid = thread_create(dummy, (void*)&arg);
  printf(1, "MAIN CONTINUED. PID = %d\n", pid);
  //join(pid);
  exit();
}

void dummy(void* arg)
{
  printf(1, "CLONE SUCCESS! ARG = %d\n", *(int*)arg);
  exit();
}
