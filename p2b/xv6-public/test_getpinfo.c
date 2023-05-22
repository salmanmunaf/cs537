#include "types.h"
#include "user.h"
#include "pstat.h"

#define TOTAL 64

int
main(int argc, char *argv[])
{
  struct pstat p;
  for (int i = 0; i < TOTAL; i++) {
    p.inuse[i] = 0;
    p.tickets[i] = 0;
    p.pid[i] = -1;
    p.ticks[i] = 0;
  }
  int r = getpinfo(&p);
  if (r == 0) {
    for(int i = 0; i < TOTAL; i++)
    {
      printf(1, "Process: inuse %d, tickets %d, pid %d, ticks %d\n", p.inuse[i], p.tickets[i], p.pid[i], p.ticks[i]);
    }
  } else {
    printf(1, "Test Failed");
  }

  exit();
}