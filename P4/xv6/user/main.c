#include "types.h"
#include "stat.h"
#include "user.h"
#include "thread.h"
#include "conc.h"

void dummy(void* arg);
int global = 5;

int main(int argc, char *argv[])
{
  int pid;
  int arg = 5;

  pid = thread_create(dummy, (void*)&arg);
  printf(1, "MAIN CONTINUED. PID = %d\n", pid);
  //join(pid);
  //while(global != 5);
  //sleep(100);
  exit();
}

void dummy(void* arg)
{
  printf(1, "CLONE SUCCESS! ARG = %d\n", *(int*)arg);
  //global = 3;
  exit();
}
