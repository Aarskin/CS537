#ifndef _TYPES_H_
#define _TYPES_H_

// Type definitions

typedef unsigned int   uint;
typedef unsigned short ushort;
typedef unsigned char  uchar;
typedef uint pde_t;
#ifndef NULL
#define NULL (0)
#endif

typedef struct
{
  int ticket;
  int turn;
  int pid;
} lock_t;

typedef struct
{
  struct pidBlock* head;
} cond_t;

struct pidBlock
{
  int pid;
  struct pidBlock* next;
};

#endif //_TYPES_H_
