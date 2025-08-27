/*
 * To be compiled with -march=armv8.5-a+memtag
 */
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/auxv.h>
#include <sys/mman.h>
#include <sys/prctl.h>

/*
 * From arch/arm64/include/uapi/asm/hwcap.h
 */
#define HWCAP2_MTE              (1 << 18)

/*
 * From arch/arm64/include/uapi/asm/mman.h
 */
#ifndef PROT_MTE
#define PROT_MTE 0x20
#endif 

/*
 * From include/uapi/linux/prctl.h
 */
#define PR_SET_TAGGED_ADDR_CTRL 55
#define PR_GET_TAGGED_ADDR_CTRL 56
# define PR_TAGGED_ADDR_ENABLE  (1UL << 0)
# define PR_MTE_TCF_SHIFT       1
# define PR_MTE_TCF_NONE        (0UL << PR_MTE_TCF_SHIFT)
# define PR_MTE_TCF_SYNC        (1UL << PR_MTE_TCF_SHIFT)
# define PR_MTE_TCF_ASYNC       (2UL << PR_MTE_TCF_SHIFT)
# define PR_MTE_TCF_MASK        (3UL << PR_MTE_TCF_SHIFT)
# define PR_MTE_TAG_SHIFT       3
# define PR_MTE_TAG_MASK        (0xffffUL << PR_MTE_TAG_SHIFT)

/*
 * Insert a random logical tag into the given pointer.
 */
#define insert_random_tag(ptr) ({                       \
        uint64_t __val;                                 \
        asm("irg %0, %1" : "=r" (__val) : "r" (ptr));   \
        __val;                                          \
})

/*
 * Set the allocation tag on the destination address.
 */
#define set_tag(tagged_addr) do {                                      \
        asm volatile("stg %0, [%0]" : : "r" (tagged_addr) : "memory"); \
} while (0)

#define NOP0
#define NOP1 "add x9, x9, #1\n"
#define NOP2 NOP1 NOP1
#define NOP4 NOP2 NOP2
#define NOP8 NOP4 NOP4

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

#define LOOP_MAX 100000000

uint64_t repeat_irg_stg(uint64_t L, void *page) {
  sleep(3);

  uint64_t start = rdtsc(), end;
  asm (
      "cmp %0, #1\n"
      "b.le 1f\n"
      "2:\n"
      "irg %1, %1\n"
      "stg %1, [%1]\n"
      "subs %0, %0, #1\n"
      "b.ne 2b\n"
      "1:\n"
      :
      : "r"(L), "r"(page)
      :
      );
  end = rdtsc() - start;
  return end;
}

uint64_t repeat_irg(uint64_t L, void *page) {
  sleep(3);

  uint64_t start = rdtsc(), end;
  asm (
      "cmp %0, #1\n"
      "b.le 1f\n"
      "2:\n"
      "irg %1, %1\n"
      "subs %0, %0, #1\n"
      "b.ne 2b\n"
      "1:\n"
      :
      : "r"(L), "r"(page)
      :
      );
  end = rdtsc() - start;
  return end;
}

uint64_t repeat_stg_sametag(uint64_t L, void *page) {
  sleep(3);

  page = (void *)((uintptr_t)page | ((uintptr_t)0xf << 56));

  uint64_t start = rdtsc(), end;
  asm (
      "cmp %0, #1\n"
      "b.le 1f\n"
      "2:\n"
      "stg %1, [%1]\n"
      "subs %0, %0, #1\n"
      "b.ne 2b\n"
      "1:\n"
      :
      : "r"(L), "r"(page)
      :
      );
  end = rdtsc() - start;
  return end;
}

uint64_t repeat_store(uint64_t L, void *page) {
  sleep(3);

  page = (void *)((uintptr_t)page | ((uintptr_t)0xf << 56));
  set_tag(page);

  uint64_t val = 1;
  uint64_t start = rdtsc(), end;
  asm (
      "cmp %0, #1\n"
      "b.le 1f\n"
      "2:\n"
      "str %2, [%1]\n"
      "subs %0, %0, #1\n"
      "b.ne 2b\n"
      "1:\n"
      :
      : "r"(L), "r"(page), "r"(val)
      :
      );
  end = rdtsc() - start;
  return end;
}

uint64_t repeat_flush_stg_sametag(uint64_t L, void *page) {
  sleep(3);

  uintptr_t tags[16];
  for (int i = 0; i < 16; i++) {
	  tags[i] = (uintptr_t)0xf << 56;
  }
  uintptr_t p = ((uintptr_t)page | tags[0]);

  uint64_t start = rdtsc(), end;
  for (uint64_t i = 0; i < L; i++) {
    __builtin___clear_cache(p, p+16);
    p = ((uintptr_t)page | tags[i % 16]);
    set_tag(p);
  }
  end = rdtsc() - start;
  return end;
}

uint64_t repeat_flush_stg_altertag(uint64_t L, void *page) {
  sleep(3);

  uintptr_t tags[16];
  for (int i = 0; i < 16; i++) {
	  tags[i] = (uintptr_t)i << 56;
  }
  uintptr_t p = ((uintptr_t)page | tags[0]);

  uint64_t start = rdtsc(), end;
  for (uint64_t i = 0; i < L; i++) {
    __builtin___clear_cache(p, p+16);
    p = ((uintptr_t)page | tags[i % 16]);
    set_tag(p);
  }
  end = rdtsc() - start;
  return end;
}

uint64_t repeat_flush_store(uint64_t L, void *page) {
  sleep(3);

  uintptr_t p = ((uintptr_t)page | ((uintptr_t)0xf << 56));
  set_tag(p);

  uint64_t val = 1;
  uint64_t start = rdtsc(), end;
  for (uint64_t i = 0; i < L; i++) {
    __builtin___clear_cache(p, p+16);
    asm("str %0, [%1]\n" : : "r" (val), "r" (p) : "memory");
  }
  end = rdtsc() - start;
  return end;
}


int main(int argc, char** argv) {
  size_t bits = atoi(argv[1]);
  /*
   * 0 == skip prctl, skip PROT_MTE
   * 1 == call prctl, skip PROT_MTE
   * 2 == skip prctl, call mprotect with PROT_MTE
   * 3 == call prctl, call mprotect with PROT_MTE
   */

  unsigned long page_sz = sysconf(_SC_PAGESIZE);
  unsigned long hwcap2 = getauxval(AT_HWCAP2);

  /* check if MTE is present */
  if (!(hwcap2 & HWCAP2_MTE))
    return EXIT_FAILURE;

  // prctl
  if (bits & 0x1) {
    if (prctl(PR_SET_TAGGED_ADDR_CTRL,
          PR_TAGGED_ADDR_ENABLE | PR_MTE_TCF_SYNC |
          (0xfffe << PR_MTE_TAG_SHIFT),
          0, 0, 0)) {
      perror("prctl() failed");
      return EXIT_FAILURE;
    }
  } else {
  }

  // PROT_MTE
  void *page = mmap(0, page_sz, PROT_READ | PROT_WRITE | (bits & 0x2 ? PROT_MTE : 0), MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (page == MAP_FAILED) {
    perror("mmap() failed");
    return EXIT_FAILURE;
  }

  uint64_t L = LOOP_MAX;
  printf("irg+stg: %lu\n", repeat_irg_stg(L, page));
  printf("irg: %lu\n", repeat_irg(L, page));
  printf("stg: %lu\n", repeat_stg_sametag(L, page));
  printf("store:%lu\n", repeat_store(L, page));
  printf("clflush+stg(onecolor): %lu\n", repeat_flush_stg_sametag(L, page));
  printf("clflush+stg(multicolor): %lu\n", repeat_flush_stg_altertag(L, page));
  printf("clflush+store: %lu\n", repeat_flush_store(L, page));

}

