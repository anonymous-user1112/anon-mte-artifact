#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NoB  
#define ISB "isb \n" 
#define DSB "dsb st \n" 
#define DMBST "dmb st \n" 
#define DMBSTLD "dmb st \n dmb ld \n" 
#define DMBSY "dmb SY \n" 
#define DMBINST "dmb NSHST \n" 
#define DMBINSY "dmb NSH \n" 

#define SB  "sb \n" 

#ifndef BARRIER
#define BARRIER NoB
#endif

#define NOP0 
#define NOP1 "add x9, x9, #1\n"
#define NOP2 NOP1 NOP1
#define NOP4 NOP2 NOP2
#define NOP5 NOP4 NOP1
#define NOP6 NOP4 NOP2
#define NOP7 NOP6 NOP1
#define NOP8 NOP4 NOP4
#define NOP9 NOP4 NOP5
#define NOP10 NOP8 NOP2
#define NOP11 NOP7 NOP4
#define NOP12 NOP8 NOP4
#define NOP16 NOP8 NOP8
#define NOP20 NOP16 NOP4
#define NOP24 NOP20 NOP4
#define NOP28 NOP24 NOP4
#define NOP32 NOP28 NOP4

#ifndef NNOP
#define NNOP NOP0
#endif

#define NOFLUSH 
#define FLUSHCACHE    "dc cvac, %0 \n"
#define FLUSHCACHETAG "dc cigdvac, %0 \n"

#ifndef FLUSH
#define FLUSH NOFLUSH
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
  if (argc != 3)
    return 1;
  long loop = atol(argv[1]);
  long outer_loop = atol(argv[2]);

  int *p  = (int*)malloc(sizeof(int) * 4);
  p += 1; // &p[1];

  unsigned long long start, end;
  for(int i = 0; i<outer_loop; i++){
    start = rdtsc();
    asm volatile (
        "cmp %1, #1\n"
        "blt 2f\n"
        "mov x0, %1\n"
        "1:\n"
        NNOP
        FLUSH
        "str w4, [%0]\n" 	// single point store
        BARRIER
        "subs x0, x0, #1\n"
        "b.ne 1b\n"
        "2:\n"
        :
        : "r"(p), "r"(loop)
        : "x0", "x4", "memory"
        );
    end = rdtsc() - start;
    printf("%llu\n", end);
  }
  
  return 0;

}
