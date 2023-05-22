#include "types.h"
#include "user.h"
#include "pstat.h"

#define NUM_CHILD 3

int
main(int argc, char *argv[])
{
  int child_pids[NUM_CHILD];

  for (int i = 0; i < NUM_CHILD; i++) {
    child_pids[i] = fork();

    if (child_pids[i] < 0) {
      // fork failed
      printf(2, "Fork child process %d failed\n", i);
      exit();
    }
    else if (child_pids[i] == 0) {
      if((fd = open(argv[0], 0)) < 0){
        printf(1, "cannot open %s\n", argv[0]);
        exit();
      }
      while((n = read(fd, buf, sizeof(buf))) > 0){
        printf(1, "reading file thread: %d", i);
      }
      close(fd);
      exit();
    }
  }
  sleep(1000);

  for (int i = 0; i < NUM_CHILD; i++) {
    kill(child_pids[i]);
    wait();
  }

  exit();
}