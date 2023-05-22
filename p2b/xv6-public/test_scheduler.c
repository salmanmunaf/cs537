#include "types.h"
#include "user.h"
#include "pstat.h"

#define TOTAL 64
#define NUM_CHILD 3

int
main(int argc, char *argv[])
{
  int child_pids[NUM_CHILD];
  int ticks[NUM_CHILD];

  for (int i = 0; i < NUM_CHILD; i++) {
    child_pids[i] = fork();

    if (child_pids[i] < 0) {
      // fork failed
      printf(2, "Fork child process %d failed\n", i);
      exit();
    }
    else if (child_pids[i] == 0) {
      int r;
      if (i == 0) {
        r = settickets(30);
      } else if (i == 1) {
        r = settickets(20);
      } else {
        r = settickets(10);
      }

      if (r != 0) {
        printf(1, "settickets for child %d Failed\n", i);
        exit();
      }
      printf(1, "settickets for child %d Succeeded\n", i);
      while (1)
        ;
      exit();
    }
  }
  sleep(1000);
  struct pstat info;
  int r = getpinfo(&info);

  for (int i = 0; i < NUM_CHILD; i++) {
    kill(child_pids[i]);
    wait();
  }
  
  if (r == 0) {
    printf(1, "getpinfo Succeeded\n");
    for(int i = 0; i < NUM_CHILD; i++) {
      for(int j = 0; j < TOTAL; j++) {
        if (info.inuse[i] && child_pids[i] == info.pid[j]) {
          ticks[i] = info.ticks[j];
        }
      }
    }

    for(int i = 0; i < NUM_CHILD; i++) {
      printf(1, "Process: i: %d, ticks %d, pid: %d\n", i, ticks[i], child_pids[i]);
    }

    if ((ticks[0]/ticks[2]) >= 3 && (ticks[1]/ticks[2]) >= 2)
      printf(1, "Test Passed\n");
    else
      printf(1, "Test Failed\n");
  } else {
    printf(1, "getpinfo Failed\n");
  }

  exit();
}