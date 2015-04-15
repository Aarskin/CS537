int thread_create(void (*start_routine)(void*), void *arg);
int thread_join(int pid);
