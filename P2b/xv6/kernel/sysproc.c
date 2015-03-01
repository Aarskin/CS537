#include "types.h"
#include "x86.h"
#include "defs.h"
#include "param.h"
#include "mmu.h"
#include "proc.h"
#include "sysfunc.h"


int getpinfo(struct pstat* )
{
  struct ptable *pt = getPtable();  

  acquire(&ptable.lock);
  
  struct proc *p;
  //make array of pstat's
  pstat *pstatA = (pstat*) malloc(NPROC * sizeof(struct pstat));
  

  int i;
  //printf("PID \t Stride \t tickets \t pass \t n_schedule \t name \n " );
  for(i = 0; i < NPROC; i++ )
    //(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    {
      p = pt.proc[i];

      pstatA[i]->pid = p->pid;
      pstatA[i]->stride = p->stride;
      pstatA[i]->name = p->name;
      pstatA[i]->tickets = p->tickets;
      pstatA[i]->pass = p->pass;
      pstatA[i]->n_schedule = p->n_schedule;

      //print out the stats to stdout
      printf("PID %i; \t stride %i \t tickets %i; \t pass %i; \t n_schedule %i; \t name %s \n", p->pid, p->stride, p->tickets, p->pass, p->n_schedule, p->name );

    }
  release(&ptable.lock);
  free(pstatA);
}



int
sys_fork(void)
{
  return fork();
}

int
sys_exit(void)
{
  exit();
  return 0;  // not reached
}

int
sys_wait(void)
{
  return wait();
}

int
sys_kill(void)
{
  int pid;

  if(argint(0, &pid) < 0)
    return -1;
  return kill(pid);
}

int
sys_getpid(void)
{
  return proc->pid;
}

int
sys_sbrk(void)
{
  int addr;
  int n;

  if(argint(0, &n) < 0)
    return -1;
  addr = proc->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

int
sys_sleep(void)
{
  int n;
  uint ticks0;
  
  if(argint(0, &n) < 0)
    return -1;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(proc->killed){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

// return how many clock tick interrupts have occurred
// since boot.
int
sys_uptime(void)
{
  uint xticks;
  
  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

int sys_settickets(void)
{
	int tickets;
	
	if(argint(0, &tickets) < 0)
	{
		return -1;
	}
	
	return tickets;
/*
	if(tickets % 10 != 0 || tickets < 10 || tickets > 150)
	{
		return -1; // Terrible, terrible input
	}
*/	
	return 5; // Replace with logic!
}

int sys_getpinfo(void)
{
	return 5; // Replace with logic!
}
