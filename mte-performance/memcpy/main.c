/*
 * To be compiled with -march=armv8.5-a+memtag
 */
#include <string.h>
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

#ifdef DOFLUSH
#define FLUSH(s, e, g) clflush(s, e, g)
#else
#define FLUSH(...)
#endif
/*
 * Insert a random logical tag into the given pointer.
 */
#define insert_random_tag(ptr) ({                       \
    uint64_t __val;                                 \
    asm("irg %0, %1" : "=r" (__val) : "r" (ptr));   \
    __val;                                          \
    })

#define tagp(void_ptr, color) (void *)((uintptr_t)void_ptr | ((uintptr_t)color << 56))

/*
 * Set the allocation tag on the destination address.
 */
#define set_tag(tagged_addr) do {                                      \
  asm volatile("stg %0, [%0]" : : "r" (tagged_addr) : "memory"); \
} while (0)

#define set_tag2(tagged_addr) do {                                  \
  asm volatile("st2g %0, [%0]" : : "r" (tagged_addr) : "memory"); \
} while (0)

#define set_tag_multi(tagged_addr) do {                                  \
  asm volatile("dc gva, %0" : : "r" (tagged_addr) : "memory"); \
} while (0)

#define timer_start() ({                                      \
    unsigned long long val; \
    asm volatile(    \
        "mrs %0, cntvct_el0\n\t" \
        "dsb sy\n\t" \
        "isb\n\t" \
        : "=r" (val)); \
        val; \
        })

#define timer_end() ({                                      \
    unsigned long long val; \
    asm volatile(    \
        "dsb sy\n\t" \
        "isb\n\t" \
        "mrs %0, cntvct_el0\n\t" \
        : "=r" (val)); \
        val; \
        })

static inline uintptr_t get_dcache_line_size() {
  uintptr_t LineSize = 0, Tmp;
  __asm__ __volatile__(
      "DCZID .req %[Tmp] \n\t"
      "mrs DCZID, dczid_el0 \n\t"
      "tbnz DCZID, #4, 3f \n\t"
      "and DCZID, DCZID, #15 \n\t"
      "mov %[LineSize], #4 \n\t"
      "lsl %[LineSize], %[LineSize], DCZID \n\t"
      ".unreq DCZID \n\t"
      "3:\n\t"
      :[LineSize] "=&r"(LineSize), [Tmp] "=&r"(Tmp)
      :
      : "memory"
      );
  return LineSize;
}

static inline void glibc_set_tags(void *tagged_ptr, size_t len) {
    uintptr_t begin = (uintptr_t) tagged_ptr;
    uintptr_t end = begin + len;
    size_t linesize, tmp;

    __asm__ __volatile__(
        ".arch_extension memtag\n"
        // Compare count with 96
        "cmp %[Len], #96\n"
        "b.hi 1f\n"   // If count > 96, jump to set_long

        // Check if count has bit 6 set (count >= 64)
        "tbnz %[Len], #6, 2f\n"

        // Set 0, 16, 32, or 48 bytes
        "lsr %[Tmp], %[Len], #5\n"
        "add %[Tmp], %[Cur], %[Tmp], lsl #4\n"
        "cbz %[Len], 3f\n"
        "stg %[Cur], [%[Cur]]\n"
        "stg %[Cur], [%[Tmp]]\n"
        "stg %[Cur], [%[End], #-16]\n"

    // Set 64..96 bytes: Write 64 bytes from the start, 32 bytes from the end
    "2:\n"
        "st2g %[Cur], [%[Cur]]\n"
        "st2g %[Cur], [%[Cur], #32]\n"
        "st2g %[Cur], [%[End], #-32]\n"
        "b 3f\n"

    // Set > 96 bytes
    "1:\n"
        "cmp %[Len], #160\n"
        "b.lo 4f\n"  // If count < 160, go to no_zva

        // ZVA optimization
        "st2g %[Cur], [%[Cur]]\n"
        "st2g %[Cur], [%[Cur], #32]\n"
        "bic %[Tmp], %[Cur], #63\n"
        "sub %[Len], %[End], %[Tmp]\n" // Adjust count
        "sub %[Len], %[Len], #128\n"

    "5:\n"
        "add %[Tmp], %[Tmp], #64\n"
        "dc gva, %[Tmp]\n"
        "subs %[Len], %[Len], #64\n"
        "b.hi 5b\n"
        "st2g %[Cur], [%[End], #-64]\n"
        "st2g %[Cur], [%[End], #-32]\n"
        "b 3f\n"

    // No ZVA case
    "4:\n"
        "sub %[Tmp], %[Cur], #32\n"
        "sub %[Len], %[Len], #64\n"
    "6:\n"
        "st2g %[Cur], [%[Tmp], #32]\n"
        "st2g %[Cur], [%[Tmp], #64]!\n"
        "subs %[Len], %[Len], #64\n"
        "b.hi 6b\n"
        "st2g %[Cur], [%[End], #-64]\n"
        "st2g %[Cur], [%[End], #-32]\n"
    "3:\n"
        : [Len] "+&r"(len), [Cur] "+&r"(begin), [Tmp] "=&r"(tmp)
        : [End] "r"(end)
        : "memory"
    );
}

static inline uintptr_t scudo_set_tags(void *tagged_ptr, size_t len, uintptr_t LineSize) {
  uintptr_t Begin = (uintptr_t) tagged_ptr;
  uintptr_t End = Begin + len;
  uintptr_t Next, Tmp;
  __asm__ __volatile__(
      ".arch_extension memtag \n\t"

      "Size .req %[Tmp] \n\t"
      "sub Size, %[End], %[Cur] \n\t"
      "cmp Size, %[LineSize], lsl #1 \n\t"
      "b.lt 3f \n\t"
      ".unreq Size \n\t"

      "LineMask .req %[Tmp] \n\t"
      "sub LineMask, %[LineSize], #1 \n\t"

      "orr %[Next], %[Cur], LineMask \n\t"

      "1:\n\t"
      "stg %[Cur], [%[Cur]], #16 \n\t"
      "cmp %[Cur], %[Next] \n\t"
      "b.lt 1b \n\t"

      "bic %[Next], %[End], LineMask \n\t"
      ".unreq LineMask \n\t"

      "2: \n\t"
      "dc gva, %[Cur] \n\t"
      "add %[Cur], %[Cur], %[LineSize] \n\t"
      "cmp %[Cur], %[Next] \n\t"
      "b.lt 2b \n\t"

      "3: \n\t"
      "cmp %[Cur], %[End] \n\t"
      "b.ge 4f \n\t"
      "stg %[Cur], [%[Cur]], #16 \n\t"
      "b 3b \n\t"

      "4: \n\t"

      : [Cur] "+&r"(Begin), [Next] "=&r"(Next), [Tmp] "=&r"(Tmp)
      : [End] "r"(End), [LineSize] "r"(LineSize)
         : "memory"
           );
  return Begin;
}

static inline __attribute__((always_inline)) void clflush(void* start, void *end, uintptr_t linesize) {
  for (uintptr_t p = (uintptr_t)start;  p < (uintptr_t)end; p += linesize) {
    asm volatile("dc cigdvac, %0" :: "r"(p));
  }
  asm volatile("dsb ish");
  asm volatile("isb");
}

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

#define RNDUP(pgsz, sz)  (((sz)+pgsz-1) & ~(pgsz-1))

typedef struct {
  unsigned long long memcpy_t;
  unsigned long long stg_t;
  unsigned long long st2g_t;
  unsigned long long dc_gva_t;
  unsigned long long scudo_t;
  unsigned long long glibc_t;
} Result;

#define ITER 10000000

static inline void tag_with_stg(void *tagged_ptr, size_t size) {
  for (uintptr_t p = (uintptr_t)tagged_ptr; p < (uintptr_t)tagged_ptr + size; p += 16) {
    set_tag(p);
  }
}
static inline void tag_with_st2g(void *tagged_ptr, size_t size) {
  for (uintptr_t p = (uintptr_t)tagged_ptr; p < (uintptr_t)tagged_ptr + size; p += 32) {
    set_tag2(p);
  }
}
static inline void tag_with_dcgva(void *tagged_ptr, size_t size, uintptr_t linesize) {
  for (uintptr_t p = (uintptr_t)tagged_ptr; p < (uintptr_t)tagged_ptr + size; p += linesize) {
    set_tag_multi(p);
  }
}

int main(int argc, char** argv) {
  if (argc != 2)
    return 1;
  unsigned long bytes = atoi(argv[1]);

  unsigned long page_sz = sysconf(_SC_PAGESIZE);
  unsigned long hwcap2 = getauxval(AT_HWCAP2);

  /* check if MTE is present */
  if (!(hwcap2 & HWCAP2_MTE))
    return EXIT_FAILURE;

  // prctl
  if (prctl(PR_SET_TAGGED_ADDR_CTRL,
        PR_TAGGED_ADDR_ENABLE | PR_MTE_TCF_SYNC | PR_MTE_TCF_ASYNC |
        (0xfffe << PR_MTE_TAG_SHIFT),
        0, 0, 0)) {
    perror("prctl() failed");
    return EXIT_FAILURE;
  }

  printf("ITER: %d\t", ITER);

  unsigned long mapsize = RNDUP(page_sz, bytes);
  Result total = {0, 0, 0, 0};
  void *p1, *tmp;
  uintptr_t linesize = get_dcache_line_size();
  unsigned long long start = 0, end = 0;

  ////////////////////////////////////////////////////////////////////////////////////////////////
  p1 = mmap(0, mapsize, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (p1 == MAP_FAILED) {
    perror("mmap() failed");
    exit(1);
  }
  tmp = mmap(0, mapsize, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (tmp == MAP_FAILED) {
    perror("mmap() failed");
    exit(1);
  }

  // buf init
  for(size_t i = 0; i < mapsize; i++)
    *((unsigned char*)p1 + i) = rand() % 256;
  for(size_t i = 0; i < mapsize; i++)
    *((unsigned char*)tmp + i) = rand() % 256;

  clflush(p1, p1 + bytes, linesize);
  clflush(tmp, tmp + bytes, linesize);
  start = timer_start();
  for (int i = 0; i < 100; i++) {
    memcpy(tmp, p1, bytes);
  }
  end = timer_end() - start;
  start = timer_start();
  for (int i = 0; i < ITER; i++) {
    memcpy(tmp, p1, bytes);
  }
  end = timer_end() - start;
  total.memcpy_t = end;
  munmap(p1, mapsize);
  munmap(tmp, mapsize);
  ////////////////////////////////////////////////////////////////////////////////////////////////
  ////////////////////////////////////////////////////////////////////////////////////////////////
  p1 = mmap(0, mapsize, PROT_READ | PROT_WRITE | PROT_MTE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (p1 == MAP_FAILED) {
    perror("mmap() failed");
    exit(1);
  }
  tmp = mmap(0, mapsize, PROT_READ | PROT_WRITE | PROT_MTE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (tmp == MAP_FAILED) {
    perror("mmap() failed");
    exit(1);
  }

  // buf init
  for(size_t i = 0; i < mapsize; i++)
    *((unsigned char*)p1 + i) = rand() % 256;

  clflush(p1, p1 + bytes, linesize);
  start = timer_start();
  for (int i = 0; i < 100; i++) {
    tag_with_stg(tagp(p1, i % 16), bytes);
  }
  end = timer_end() - start;
  start = timer_start();
  for (int i = 0; i < ITER; i++) {
    FLUSH(p1, p1 + bytes, linesize);
    tag_with_stg(tagp(p1, i % 16), bytes);
  }
  end = timer_end() - start;
  total.stg_t = end;
  munmap(p1, mapsize);
  ////////////////////////////////////////////////////////////////////////////////////////////////
  ////////////////////////////////////////////////////////////////////////////////////////////////
  p1 = mmap(0, mapsize, PROT_READ | PROT_WRITE | PROT_MTE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (p1 == MAP_FAILED) {
    perror("mmap() failed");
    exit(1);
  }
  tmp = mmap(0, mapsize, PROT_READ | PROT_WRITE | PROT_MTE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (tmp == MAP_FAILED) {
    perror("mmap() failed");
    exit(1);
  }

  // buf init
  for(size_t i = 0; i < mapsize; i++)
    *((unsigned char*)p1 + i) = rand() % 256;

  clflush(p1, p1 + bytes, linesize);
  start = timer_start();
  for (int i = 0; i < 100; i++) {
    tag_with_st2g(tagp(p1, i % 16), bytes);
  }
  end = timer_end() - start;
  start = timer_start();
  for (int i = 0; i < ITER; i++) {
    FLUSH(p1, p1 + bytes, linesize);
    tag_with_st2g(tagp(p1, i % 16), bytes);
  }
  end = timer_end() - start;
  total.st2g_t = end;
  munmap(p1, mapsize);
  ////////////////////////////////////////////////////////////////////////////////////////////////
  ////////////////////////////////////////////////////////////////////////////////////////////////
  p1 = mmap(0, mapsize, PROT_READ | PROT_WRITE | PROT_MTE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (p1 == MAP_FAILED) {
    perror("mmap() failed");
    exit(1);
  }
  tmp = mmap(0, mapsize, PROT_READ | PROT_WRITE | PROT_MTE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (tmp == MAP_FAILED) {
    perror("mmap() failed");
    exit(1);
  }

  // buf init
  for(size_t i = 0; i < mapsize; i++)
    *((unsigned char*)p1 + i) = rand() % 256;

  clflush(p1, p1 + bytes, linesize);
  start = timer_start();
  for (int i = 0; i < 100; i++) {
    tag_with_dcgva(tagp(p1, i % 16), bytes, linesize);
  }
  end = timer_end() - start;
  start = timer_start();
  for (int i = 0; i < ITER; i++) {
    FLUSH(p1, p1 + bytes, linesize);
    tag_with_dcgva(tagp(p1, i % 16), bytes, linesize);
  }
  end = timer_end() - start;
  total.dc_gva_t = end;
  munmap(p1, mapsize);
  ////////////////////////////////////////////////////////////////////////////////////////////////
  ////////////////////////////////////////////////////////////////////////////////////////////////
  p1 = mmap(0, mapsize, PROT_READ | PROT_WRITE | PROT_MTE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (p1 == MAP_FAILED) {
    perror("mmap() failed");
    exit(1);
  }
  tmp = mmap(0, mapsize, PROT_READ | PROT_WRITE | PROT_MTE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (tmp == MAP_FAILED) {
    perror("mmap() failed");
    exit(1);
  }

  // buf init
  for(size_t i = 0; i < mapsize; i++)
    *((unsigned char*)p1 + i) = rand() % 256;

  clflush(p1, p1 + bytes, linesize);
  start = timer_start();
  for (int i = 0; i < 100; i++) {
    scudo_set_tags(tagp(p1, i % 16), bytes, linesize);
  }
  end = timer_end() - start;
  start = timer_start();
  for (int i = 0; i < ITER; i++) {
    FLUSH(p1, p1 + bytes, linesize);
    scudo_set_tags(tagp(p1, i % 16), bytes, linesize);
  }
  end = timer_end() - start;
  total.scudo_t = end;
  munmap(p1, mapsize);
  ////////////////////////////////////////////////////////////////////////////////////////////////
  ////////////////////////////////////////////////////////////////////////////////////////////////
  p1 = mmap(0, mapsize, PROT_READ | PROT_WRITE | PROT_MTE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (p1 == MAP_FAILED) {
    perror("mmap() failed");
    exit(1);
  }
  tmp = mmap(0, mapsize, PROT_READ | PROT_WRITE | PROT_MTE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (tmp == MAP_FAILED) {
    perror("mmap() failed");
    exit(1);
  }

  // buf init
  for(size_t i = 0; i < mapsize; i++)
    *((unsigned char*)p1 + i) = rand() % 256;

  clflush(p1, p1 + bytes, linesize);
  start = timer_start();
  for (int i = 0; i < 100; i++) {
    glibc_set_tags(tagp(p1, i % 16), bytes);
  }
  end = timer_end() - start;
  start = timer_start();
  for (int i = 0; i < ITER; i++) {
    FLUSH(p1, p1 + bytes, linesize);
    glibc_set_tags(tagp(p1, i % 16), bytes);
  }
  end = timer_end() - start;
  total.glibc_t = end;
  munmap(p1, mapsize);
  ////////////////////////////////////////////////////////////////////////////////////////////////

  printf("bufsize %lu: (memcpy, stg, st2g, set_tags, scudo, glibc) = (%llu, %llu, %llu, %llu, %llu, %llu)\n", bytes, total.memcpy_t, total.stg_t, total.st2g_t, total.dc_gva_t, total.scudo_t, total.glibc_t);

  return 0;
}
