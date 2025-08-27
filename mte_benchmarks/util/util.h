#ifndef _MY_UTIL_H
#define _MY_UTIL_H

#define _GNU_SOURCE

#include <sched.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/prctl.h>
#include <sys/auxv.h>
#include <sys/mman.h>
#include <unistd.h>
#include <inttypes.h>
#include <stdio.h>
#include <sys/time.h>
#include "sha256.h"
#include <fcntl.h>			// For O_RDONLY in get_physical_addr fn 

/** compile with -std=gnu99 */
#include <stdint.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <linux/perf_event.h>


// For MTE
#define PROT_MTE                 0x20
#define total_key                16

// set tag on the memory region
#define set_tag(tagged_addr) do {                                      \
        asm volatile("stg %0, [%0]" : : "r" (tagged_addr) : "memory"); \
} while (0)

// Insert with a user-defined tag
#define insert_my_tag(ptr, input_tag) \
    ((unsigned char*)((uint64_t)(ptr) | ((input_tag) << 56)))

// Insert a random tag
#define insert_random_tag(ptr) ({                       \
        uint64_t __val;                                 \
        asm("irg %0, %1" : "=r" (__val) : "r" (ptr));   \
        __val;                                          \
})

#define MEASURE_TIME(code, header_str) \
do { \
        struct timeval start, end; \
        double elapsed_time; \
        gettimeofday(&start, NULL); \
        { \
            code; \
        } \
        gettimeofday(&end, NULL); \
        elapsed_time = (end.tv_sec - start.tv_sec) * 1e6 + (end.tv_usec - start.tv_usec); \
        printf("%s time is %f us\n", header_str, elapsed_time); \
} while (0)

#define FORCE_READ_INT(var) __asm__("" ::"r"(var));

#define INST_SYNC asm volatile("ISB")
#define DATA_SYNC asm volatile("DSB SY")

#define MEM_BARRIER \
	DATA_SYNC;      \
	INST_SYNC


void pin_cpu(size_t core_ID);

int init_mte(int testmte);

unsigned char * mmap_option(int mte, uint64_t size);

void mprotect_option(int mte, unsigned char * ptr, uint64_t size);

unsigned char * mte_tag(unsigned char *ptr, unsigned long long tag, int random);

void create_random_chain(uint64_t* indices, uint64_t len);

void read_write_random_order(uint64_t* indices, uint64_t len, uint64_t** ptr, int workload_iter);

void read_read_dependency(uint64_t** ptr, uint64_t count, int workload_iter);

void write_random_order(uint64_t* indices, uint64_t* ptr, uint64_t count, int workload_iter);

void read_random_order(uint64_t* indices, uint64_t* ptr, uint64_t count, int workload_iter);

void store_load_random_order(uint64_t* indices, uint64_t* ptr, uint64_t count, int workload_iter);

void write_read_random_order(uint64_t* indices, uint64_t* ptr, uint64_t count, int workload_iter);

void sha256_ctx(SHA256_CTX* ctx, const unsigned char * str, uint64_t size, unsigned char hash[], int workload_iter);

void read_seq_only(uint64_t* ptr, uint64_t size, int workload_iter);

void write_seq_only(uint64_t* ptr, uint64_t size, int workload_iter);

void read_write_seq_only(uint64_t* ptr, uint64_t size, int workload_iter);

uint64_t get_physical_addr(uint64_t virtual_addr);

static inline void
arm_clean_va_to_poc(void const *p __attribute__((unused)))
{
	asm volatile("dc cvac, %0" : : "r" (p) : "memory");
}

void init(void);

void fini(void);

long long cpucycles(void);

#endif