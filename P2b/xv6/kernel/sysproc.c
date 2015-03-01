#include "types.h"
#include "x86.h"
#include "defs.h"
#include "param.h"
#include "mmu.h"
#include "proc.h"
#include "sysfunc.h"
#include "spinlock.h"
//#include "user.h"

extern struct {
  struct spinlock lock;
  struct proc proc[NPROC];
} ptable;

int sys_getpinfo(void)
{
  struct pstat* pstatA;

  if(argptr(1, (char**)&pstatA, sizeof(struct pstat)) < 0)
  {
  	 cprintf("didn't parse arg");
  	return -1;
  }

  //struct ptable *pt = ptable;  

  acquire(&ptable.lock);
  
  struct proc* p;
  //make array of pstat's
  //struct pstat* pstatA = (struct pstat*) malloc((uint)(NPROC * sizeof(struct pstat)));
  

  int i = 0;
  //printf("PID \t Stride \t tickets \t pass \t n_schedule \t name \n ");
  for/*(i = 0; i < NPROC; i++ )
     */(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    { 
    
      p = &ptable.proc[i];
	 
      (pstatA + i)->pid = p->pid;
      (pstatA + i)->stride = p->stride;
      strncpy((pstatA+i)->name, p->name, 16);//(pstatA + i)->name = p->name;
      (pstatA + i)->tickets = p->tickets;
      (pstatA + i)->pass = p->pass;
      (pstatA + i)->n_schedule = p->n_schedule;
      if((p->state) != UNUSED  )
	      (pstatA + i)->inuse = 1;
      
		 //print out the stats to stdout
	  if((p->state) != UNUSED)
		 cprintf("PID %d; \t stride %d \t tickets %d; \t pass %d; \t n_schedule %d; \t name %s \n", p->pid, p->stride, p->tickets, p->pass, p->n_schedule, p->name );
      i++; // Increment index
    }
  release(&ptable.lock);
  return 0;
  //free(pstatA);
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
		return -1; // No parameter?
	}
	
	if(tickets % 10 != 0 || tickets < 10 || tickets > 150)
	{
		return -1; // Bad parameter
	}
	
	proc->tickets = tickets;		 // Assign tickets
	proc->stride  = LCM / tickets; // Update stride value 
	
	return 0; // Success!
}
