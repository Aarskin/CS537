/* multi threaded alloc and free calls */
#include <assert.h>
#include <stdlib.h>
#include <pthread.h>
#include "mymem.h"

#define MAX 100

char* buffer[MAX];
int fill = 0;
int use = 0;
int count = 0;
int loops = 1000;

void put(char *ptr)
{
	buffer[fill] = ptr;
	fill = (fill + 1) % MAX;
	count++;
}

char* get()
{
	char* tmp = buffer[use];
	use = (use + 1) % MAX;
	count--;
	return tmp;
}

pthread_cond_t *empty, *full;
pthread_mutex_t *mutex;

void* producer(void *arg)
{
	int i;
	char *nfPtr = NULL;
	for(i=0; i<loops; i++)
	{
		nfPtr = NULL;
		nfPtr = Mem_Alloc(32);
		pthread_mutex_lock(mutex);
		while (count == MAX)
			pthread_cond_wait(empty, mutex);
		assert(nfPtr != NULL);
		put(nfPtr);
		pthread_cond_signal(full);
		pthread_mutex_unlock(mutex);
	}
	return NULL;
}

void* consumer(void *arg)
{
	int i;
	char *nfPtr = NULL;
	for(i=0; i<loops; i++)
	{
		pthread_mutex_lock(mutex);
		while (count == 0)
			pthread_cond_wait(full, mutex);
		nfPtr = get();
		assert(Mem_Free(nfPtr) == 0);
		pthread_cond_signal(empty);
		pthread_mutex_unlock(mutex);
	}
	return NULL;
}


void initSync()
{
	mutex = (pthread_mutex_t *) malloc (sizeof (pthread_mutex_t));
	empty = (pthread_cond_t *) malloc (sizeof (pthread_cond_t));
	full = (pthread_cond_t *) malloc (sizeof (pthread_cond_t));

	pthread_mutex_init (mutex, NULL);	
	pthread_cond_init (full, NULL);
	pthread_cond_init (empty, NULL);
}


int main()
{
	assert(Mem_Init(8192,64) != NULL);

	initSync();

	pthread_t p1,p2,c1,c2;

	pthread_create(&p1, NULL, producer, NULL);
	pthread_create(&p2, NULL, producer, NULL);
	pthread_create(&c1, NULL, consumer, NULL);
	pthread_create(&c2, NULL, consumer, NULL);
	
	pthread_join(p1, NULL);
	pthread_join(p2, NULL);
	pthread_join(c1, NULL);
	pthread_join(c2, NULL);

	exit(0);	
}
