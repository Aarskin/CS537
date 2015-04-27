#include "types.h"
#include "defs.h"
#include "param.h"
#include "mmu.h"
#include "x86.h"
#include "proc.h"
#include "spinlock.h"

#define ANY -1

struct {
  struct spinlock lock;
  struct proc proc[NPROC];
} ptable;

static struct proc *initproc;

int nextpid = 1;
extern void forkret(void);
extern void trapret(void);

static void wakeup1(void *chan);

void
pinit(void)
{
  initlock(&ptable.lock, "ptable");
}

// Look in the process table for an UNUSED proc.
// If found, change state to EMBRYO and initialize
// state required to run in the kernel.
// Otherwise return 0.
static struct proc*
allocproc(void)
{
  struct proc *p;
  char *sp;

  acquire(&ptable.lock);
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if(p->state == UNUSED)
      goto found;
  release(&ptable.lock);
  return 0;

found:
  p->state = EMBRYO;
  p->pid = nextpid++;
  release(&ptable.lock);

  // Allocate kernel stack if possible.
  if((p->kstack = kalloc()) == 0){
    p->state = UNUSED;
    return 0;
  }
  sp = p->kstack + KSTACKSIZE;
  
  // Leave room for trap frame.
  sp -= sizeof *p->tf;
  p->tf = (struct trapframe*)sp;
  
  // Set up new context to start executing at forkret,
  // which returns to trapret.
  sp -= 4;
  *(uint*)sp = (uint)trapret;

  sp -= sizeof *p->context;
  p->context = (struct context*)sp;
  memset(p->context, 0, sizeof *p->context);
  p->context->eip = (uint)forkret;

  return p;
}

// Set up first user process.
void
userinit(void)
{
  struct proc *p;
  extern char _binary_initcode_start[], _binary_initcode_size[];
  
  p = allocproc();
  acquire(&ptable.lock);
  initproc = p;
  if((p->pgdir = setupkvm()) == 0)
    panic("userinit: out of memory?");
  inituvm(p->pgdir, _binary_initcode_start, (int)_binary_initcode_size);
  p->sz = PGSIZE;
  memset(p->tf, 0, sizeof(*p->tf));
  p->tf->cs = (SEG_UCODE << 3) | DPL_USER;
  p->tf->ds = (SEG_UDATA << 3) | DPL_USER;
  p->tf->es = p->tf->ds;
  p->tf->ss = p->tf->ds;
  p->tf->eflags = FL_IF;
  p->tf->esp = PGSIZE;
  p->tf->eip = 0;  // beginning of initcode.S

  safestrcpy(p->name, "initcode", sizeof(p->name));
  p->cwd = namei("/");

  p->state = RUNNABLE;
  release(&ptable.lock);
}

// Grow current process's memory by n bytes.
// Return 0 on success, -1 on failure.
int
growproc(int n)
{
  uint sz;
  
  acquire(&ptable.lock);
  sz = proc->sz;
  if(n > 0){
    if((sz = allocuvm(proc->pgdir, sz, sz + n)) == 0)
      return -1;
  } else if(n < 0){
    if((sz = deallocuvm(proc->pgdir, sz, sz + n)) == 0)
      return -1;
  }
  proc->sz = sz;
  
  if(proc->thread) // reflect the changes in the parent as well
    proc->parent->sz = sz;
    
  release(&ptable.lock);
  
  switchuvm(proc);
  return 0;
}

// Create a new process copying p as the parent.
// Sets up stack to return as if from system call.
// Caller must set state of returned proc to RUNNABLE.
int
fork(void)
{
  int i, pid;
  struct proc *np;

  // Allocate process.
  if((np = allocproc()) == 0)
    return -1;

  // Copy process state from p.
  if((np->pgdir = copyuvm(proc->pgdir, proc->sz)) == 0){
    kfree(np->kstack);
    np->kstack = 0;
    np->state = UNUSED;
    return -1;
  }
  np->sz = proc->sz;
  np->parent = proc;
  *np->tf = *proc->tf;

  // Clear %eax so that fork returns 0 in the child.
  np->tf->eax = 0;

  for(i = 0; i < NOFILE; i++)
    if(proc->ofile[i])
      np->ofile[i] = filedup(proc->ofile[i]);
  np->cwd = idup(proc->cwd);
 
  pid = np->pid;
  np->state = RUNNABLE;
  safestrcpy(np->name, proc->name, sizeof(proc->name));
  return pid;
}

// stack is the lowest memory addr of the allocated page
int clone(void(*fcn)(void*), void* arg, void* stack)
{ 
  int i;
  struct proc* thread; // Essentially a process to the scheduler
  struct proc* parent;
  
  // debug only
  struct proc* test = proc;
  
  // Allocate process (from fork)
  if((thread = allocproc()) == 0)
    return -1;
    
  if(!((int)stack + PGSIZE <= proc->sz))
    return -1; // Not enough allocated space for stack

  // Determine parent
  if(test/*proc*/->thread)
    parent = proc->parent;  // This is a thread
  else 
    parent = proc;          // This is orig proc
    
  // Copy open files (from fork)
  for(i = 0; i < NOFILE; i++)
    if(proc->ofile[i])
      thread->ofile[i] = filedup(proc->ofile[i]); 
    
  if((int)stack % PGSIZE == 0) // page aligned, set it up
  {
    /* Set up user stack
    -----------
    <garbage up>
    -----------
    fake return <--- stack pointer
    -----------
    argument   
    -----------
    */
    stack += PGSIZE-4; // Move stack pointer to the last word in the page
    *((int*)stack) = (int)arg; // The argument
    stack -= 4; // Move up a word
    *((int*)stack) = 0xffffffff; // Fake return
  }  
  else
    return -1; // stack pointer must be page aligned
  
      
  // Mimic the existing process' proc struct
  thread->thread  = 1; // Duh
  thread->sz      = proc->sz;
  thread->pgdir   = proc->pgdir;
  thread->parent  = parent; // Assign the parent
  *thread->tf     = *proc->tf;
  thread->chan    = 0; // indicates sleeping, which we are not
  thread->cwd     = idup(proc->cwd); // (From fork)
  thread->tf->eip = (int)fcn; // Set the entry point
  thread->tf->esp = (int)stack; // Set the stack pointer
  thread->state   = RUNNABLE; // Ready to run
  
  return thread->pid;
}

// Exit the current process.  Does not return.
// An exited process remains in the zombie state
// until its parent calls wait() to find out it exited.
void
exit(void)
{
  struct proc *p;
  int fd;

  // debug only
  struct proc* test = proc;

  if(test/*proc*/ == initproc)
    panic("init exiting");
    
  acquire(&ptable.lock);

  // Close all open files. OG process only
  if(!proc->thread)
  {
    for(fd = 0; fd < NOFILE; fd++){
      if(proc->ofile[fd]){
        fileclose(proc->ofile[fd]);
        proc->ofile[fd] = 0;
      }
    }
  }  

  iput(proc->cwd); // decrement ref count
  if(!proc->thread) // only og process can wipe this out
    proc->cwd = 0;

  // Pass abandoned children to init, kill and join if thread
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
  {
    if(p->parent == proc)
    {
      if(p->thread)
      {
        // kill
        p->killed = 1;
        
        // Wake process from sleep if necessary.
        if(p->state == SLEEPING)
          p->state = RUNNABLE;
          
        join_logic(p->pid);
      }
      else
      {
        p->parent = initproc;
        if(p->state == ZOMBIE)
          wakeup1(initproc);
      }
    }
  }
  
  // Parent might be sleeping in wait()/join().
  wakeup1(proc->parent);

  // Jump into the scheduler, never to return.
  proc->state = ZOMBIE;
  sched();
  panic("zombie exit");
}

// Wait for a child process to exit and return its pid.
// Return -1 if this process has no children.
int
wait(void)
{
  struct proc *p;
  int havekids, pid;

  acquire(&ptable.lock);
  for(;;){
    // Scan through table looking for zombie children.
    havekids = 0;
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      if(p->parent != proc || p->thread)
        continue;
      havekids = 1;
      if(p->state == ZOMBIE){
        // Found one.
        pid = p->pid;
        kfree(p->kstack);
        p->kstack = 0;
        freevm(p->pgdir);
        p->state = UNUSED;
        p->pid = 0;
        p->parent = 0;
        p->name[0] = 0;
        p->killed = 0;
        release(&ptable.lock);
        return pid;
      }
    }

    // No point waiting if we don't have any children.
    if(!havekids || proc->killed){
      release(&ptable.lock);
      return -1;
    }

    // Wait for children to exit.  (See wakeup1 call in proc_exit.)
    sleep(proc, &ptable.lock);  //DOC: wait-sleepclo
  }
}

int join_logic(int pid)
{  
  struct proc* p;
  int hasRoomie, tid; // threadID
  
  while(1)
  {
    hasRoomie = 0;
    
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    {
      if(!p->thread) // join is only interested in threads
        continue;      
      
      // Figure out if there's a valid process to join on
      if(!proc->thread) // check if p is a child thread of caller
      {
        if(p->parent != proc) // not a child
          continue;
        if(p->pid != pid && pid != ANY) // not the thread we are looking for
          continue;
      }
      else // check if p is a child thread of the same parent as the caller
      {
        // found a potential roommate
        // cannot find main process as roommate
        if(p->parent == proc->parent)
        {
          if(p->pid != pid && pid != ANY)
            continue;
          // else we found a roommate, fall out 
        }
        else // no match here
          continue;          
      }     
        
      hasRoomie = 1;
      
      // Clean up the dead roommate you found
      if(p->state == ZOMBIE)
      {
        // Found one that's already done.
        tid = p->pid;
        kfree(p->kstack);
        p->kstack = 0;
        p->state = UNUSED;
        p->pid = 0;
        p->parent = 0;
        p->name[0] = 0;
        p->killed = 0;
        return tid;
      }
    }
    
    // No point waiting if we don't have any roommate threads.
    if(!hasRoomie || proc->killed)
    {
      return -1;
    }
    
    // Found a living roommate, sleep until it's dead
    if(proc->thread)
      sleep(proc->parent, &ptable.lock);
    else
      sleep(proc, &ptable.lock);
  }
}

int join(int pid)
{
  int retpid;
  
  acquire(&ptable.lock);
  retpid = join_logic(pid);
  release(&ptable.lock);
  
  return retpid;
}

// Per-CPU process scheduler.
// Each CPU calls scheduler() after setting itself up.
// Scheduler never returns.  It loops, doing:
//  - choose a process to run
//  - swtch to start running that process
//  - eventually that process transfers control
//      via swtch back to the scheduler.
void
scheduler(void)
{
  struct proc *p;

  for(;;){
    // Enable interrupts on this processor.
    sti();

    // Loop over process table looking for process to run.
    acquire(&ptable.lock);
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      if(p->state != RUNNABLE)
        continue;

      // Switch to chosen process.  It is the process's job
      // to release ptable.lock and then reacquire it
      // before jumping back to us.
      proc = p;
      switchuvm(p);
      p->state = RUNNING;
      swtch(&cpu->scheduler, proc->context);
      switchkvm();

      // Process is done running for now.
      // It should have changed its p->state before coming back.
      proc = 0;
    }
    release(&ptable.lock);

  }
}

// Enter scheduler.  Must hold only ptable.lock
// and have changed proc->state.
void
sched(void)
{
  int intena;

  if(!holding(&ptable.lock))
    panic("sched ptable.lock");
  if(cpu->ncli != 1)
    panic("sched locks");
  if(proc->state == RUNNING)
    panic("sched running");
  if(readeflags()&FL_IF)
    panic("sched interruptible");
  intena = cpu->intena;
  swtch(&proc->context, cpu->scheduler);
  cpu->intena = intena;
}

// Give up the CPU for one scheduling round.
void
yield(void)
{
  acquire(&ptable.lock);  //DOC: yieldlock
  proc->state = RUNNABLE;
  sched();
  release(&ptable.lock);
}

// A fork child's very first scheduling by scheduler()
// will swtch here.  "Return" to user space.
void
forkret(void)
{
  // Still holding ptable.lock from scheduler.
  release(&ptable.lock);
  
  // Return to "caller", actually trapret (see allocproc).
}

// Atomically release lock and sleep on chan.
// Reacquires lock when awakened.
void
sleep(void *chan, struct spinlock *lk)
{
  if(proc == 0)
    panic("sleep");

  if(lk == 0)
    panic("sleep without lk");

  // Must acquire ptable.lock in order to
  // change p->state and then call sched.
  // Once we hold ptable.lock, we can be
  // guaranteed that we won't miss any wakeup
  // (wakeup runs with ptable.lock locked),
  // so it's okay to release lk.
  if(lk != &ptable.lock){  //DOC: sleeplock0
    acquire(&ptable.lock);  //DOC: sleeplock1
    release(lk);
  }

  // Go to sleep.
  proc->chan = chan;
  proc->state = SLEEPING;
  sched();

  // Tidy up.
  proc->chan = 0;

  // Reacquire original lock.
  if(lk != &ptable.lock){  //DOC: sleeplock2
    release(&ptable.lock);
    acquire(lk);
  }
}

void cv_sleep(lock_t* lock)
{
  acquire(&ptable.lock);
  fetchAndAdd(&lock->turn, 1); // Release lock
  sleep(proc, &ptable.lock);  
  release(&ptable.lock);
}

// Wake up all processes sleeping on chan.
// The ptable lock must be held.
static void
wakeup1(void *chan)
{
  struct proc *p;

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if(p->state == SLEEPING && p->chan == chan)
      p->state = RUNNABLE;
}

// Wake up all processes sleeping on chan.
void
wakeup(void *chan)
{
  acquire(&ptable.lock);
  wakeup1(chan);
  release(&ptable.lock);
}

void cv_wake(int pid)
{
  struct proc* p;

  acquire(&ptable.lock);
  
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
  {
    if(p->pid != pid)
    {
      wakeup1(proc);
      break;
    }
  }
  
  release(&ptable.lock);
}

// Kill the process with the given pid.
// Process won't exit until it returns
// to user space (see trap in trap.c).
int
kill(int pid)
{
  struct proc *p;

  acquire(&ptable.lock);
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->pid == pid){
      p->killed = 1;
      // Wake process from sleep if necessary.
      if(p->state == SLEEPING)
        p->state = RUNNABLE;
      release(&ptable.lock);
      return 0;
    }
  }
  release(&ptable.lock);
  return -1;
}

// Print a process listing to console.  For debugging.
// Runs when user types ^P on console.
// No lock to avoid wedging a stuck machine further.
void
procdump(void)
{
  static char *states[] = {
  [UNUSED]    "unused",
  [EMBRYO]    "embryo",
  [SLEEPING]  "sleep ",
  [RUNNABLE]  "runble",
  [RUNNING]   "run   ",
  [ZOMBIE]    "zombie"
  };
  int i;
  struct proc *p;
  char *state;
  uint pc[10];
  
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->state == UNUSED)
      continue;
    if(p->state >= 0 && p->state < NELEM(states) && states[p->state])
      state = states[p->state];
    else
      state = "???";
    cprintf("%d %s %s", p->pid, state, p->name);
    if(p->state == SLEEPING){
      getcallerpcs((uint*)p->context->ebp+2, pc);
      for(i=0; i<10 && pc[i] != 0; i++)
        cprintf(" %p", pc[i]);
    }
    cprintf("\n");
  }
}

