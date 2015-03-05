#ifndef ELSE_H
#define ELSE_H
// Necessary?
//int specialSize;
//struct FreeHeader* head;

int Dump(struct FreeHeader*, char* name);
struct AllocatedHeader* SlabAlloc(int size);
struct AllocatedHeader* NextAlloc(int size);

#endif//ELSE_H
