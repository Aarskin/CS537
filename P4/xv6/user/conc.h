// Threads
int thread_create(void (*start_routine)(void*), void *arg);
int thread_join(int pid);

// Locks
typedef struct
{
  int ticket;
  int turn;
} lock_t;

void lock_init(lock_t* lock);
void lock_acquire(lock_t* lock);
void lock_release(lock_t* lock);

// CV's
typedef struct
{
  lock_t lock;
} cond_t;

void cv_wait(cond_t*, lock_t*);
void cv_signal(cond_t*);


