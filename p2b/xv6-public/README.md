# Project 2b: Scheduling and Virtual Memory in xv6
## Part 1
- Created three system calls called `settickets`, `gettickets` and `getpinfo`.
- Added two variables in `proc` structure called `tickets` (Number of tickets) and `ticks` (The number of ticks the process has accumulated) inside `proc.h` file.
- Made changes in the scheduler and fork functions in `proc.c`.
    - In `fork` function added statements to set number of ticks to 0 and number of tickets to the same as parent process for the newly created process.
    - In `scheduler` function, added a for loop to run a process for the number of tickets it has.

## Part 2
- Initialized `sz` variable in `exec.c` to PGSIZE (4096) instead of 0 to start allocating memory from the second page.
- In `vm.c` made changes to `copyuvm` function so that it copies virtual memory from the second page.
- In `syscall.c` added a check in `argptr` function to check if the pointer passed is not less than the PGSIZE as that is not allocated.
- In `Makefile` made change to put the text of the code from `0x1000` (4096) address instead of 0.