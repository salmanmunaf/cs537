#include "types.h"
#include "user.h"

int
main(int argc, char *argv[])
{
  int *p = (int *) atoi(argv[1]);
  printf(1, "%x\n", *p);
  exit();
}
