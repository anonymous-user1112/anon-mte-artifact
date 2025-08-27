#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NOP0 
#define NOP1 "add x9, x9, #1\n"
#define NOP2 NOP1 NOP1
#define NOP4 NOP2 NOP2
#define NOP8 NOP4 NOP4
#define NOP16 NOP8 NOP8
#define NOP32 NOP16 NOP16
#define NOP64 NOP32 NOP32
#define NOP128 NOP64 NOP64
#define NOP256 NOP128 NOP128
#define NOP512 NOP256 NOP256
#define NOP1024 NOP512 NOP512

#ifndef NNOP
#define NNOP NOP0
#endif

static unsigned long long rdtsc(void)
{
  unsigned long long val;

  /*
   * According to ARM DDI 0487F.c, from Armv8.0 to Armv8.5 inclusive, the
   * system counter is at least 56 bits wide; from Armv8.6, the counter
   * must be 64 bits wide.  So the system counter could be less than 64
   * bits wide and it is attributed with the flag 'cap_user_time_short'
   * is true.
   */
  asm volatile("mrs %0, cntvct_el0" : "=r" (val));

  return val;
}
int main(int argc, char **argv) {
  if (argc != 2)
    return 1;
  long loop = atol(argv[1]);

  int *p  = malloc(sizeof(int) * 4);
  p += 1; // &p[1];

  unsigned long long start, end;
  start = rdtsc();
  asm volatile (
      "cmp %1, #1\n"
      "blt 2f\n"
      "mov x0, %1\n"
      "1:\n"
      NNOP
      "str w4, [%0]\n" 	// single point store
      "subs x0, x0, #1\n"
      "b.ne 1b\n"
      "2:\n"
      :
      : "r"(p), "r"(loop)
      : "x0", "x4", "memory"
      );
  end = rdtsc() - start;
  printf("%llu\n", end);
  //printf("%llu %d\n", end, sc);

}
