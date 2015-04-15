// Threads
int thread_create(void (*start_routine)(void*), void *arg);
int thread_join(int pid);

// Locks
struct lock_t // typedef
{
  int locked;
};

void lock_init(struct lock_t* lock);
void lock_acquire(struct lock_t* lock);
void lock_release(struct lock_t* lock);

// CV's
struct cond_t // typedef
{
  struct lock_t lock;
};

void cv_wait(struct cond_t*,struct lock_t*);
void cv_signal(struct cond_t*);


