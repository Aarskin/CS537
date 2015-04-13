#include "types.h"
#include "stat.h"
#include "user.h"

void dummy(void*);

int main(int argc, char *argv[])
{
  void* args = (void*)5;
  void* stack = (void*)35;

  clone(dummy, args, stack);  
  join(5);
  exit();
}

void dummy(void* args)
{
  return;
}
