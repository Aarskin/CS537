#ifndef _PSTAT_H_
#define _PSTAT_H_


struct pstat {
  int inuse;
  int pid;
  char name[16];
  int tickets;
  int pass;
  int stride;
  int n_schedule;
};

#endif // _PSTAT_H_
