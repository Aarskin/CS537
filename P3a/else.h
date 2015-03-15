#ifndef ELSE_H
#define ELSE_H
// Necessary?
//int specialSize;
//struct FreeHeader* head;

typedef int seg_t;
#define	FAULT	-1	
#define	SLAB		0	
#define	NEXT		1

int Dump(struct FreeHeader*, char* name);
int SlabCoalesce(void* ptr);
int NextCoalesce(void* ptr, int freeBytes);
seg_t PointerCheck(void* ptr);
struct AllocatedHeader* SlabAlloc(int size);
struct AllocatedHeader* NextAlloc(int size);

void Pthread_mutex_lock(pthread_mutex_t*);
void Pthread_mutex_unlock(pthread_mutex_t*);


#endif//ELSE_H
