#include "types.h"
#include "user.h"

int
main(int argc, char *argv[])
{
  int rc = fork();
  if (rc < 0) {
      // fork failed
      printf(1, "fork failed");
  }
  else if (rc == 0) {
    int r = settickets(4);
    printf(1, "r: %d\n", r);
    if (r == 0) {
      int tickets = gettickets();
      printf(1, "tickets: %d\n", tickets);
      if (tickets == 4) {
        printf(1, "Test Passed\n");
      } else {
        printf(1, "Test Failed\n");
      }
    } else {
      printf(1, "Test Failed\n");
    }
  } else {
    wait();
  }
  
  exit();
}