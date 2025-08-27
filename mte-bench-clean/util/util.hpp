#ifndef UTIL_HPP
#define UTIL_HPP

#include <stdio.h>   // For printf
#include <stdlib.h>  // For exit and EXIT_FAILURE
#include <sched.h>
#include <sys/prctl.h>
#include <sys/auxv.h>
#include <sys/mman.h>

#define FORCE_READ_INT(var) __asm__("" ::"r"(var));

#define DATA_SYNC_ANY asm volatile("DSB SY")
#define INST_SYNC asm volatile("ISB")
#define MEM_BARRIER \
	DATA_SYNC_ANY;      \
	INST_SYNC

#define PROT_MTE                 0x20

#define DATA_SYNC_ST asm volatile("DSB ST")
#define DMB_SYNC_ST asm volatile("DMB ST")

unsigned long long rdtsc(void);

#define MEASURE_CYCLE(code, header_str) \
do { \
        MEM_BARRIER;\
        uint64_t begin = rdtsc(); \
        MEM_BARRIER;\
        { \
            code; \
        } \
        MEM_BARRIER;\
        uint64_t end = rdtsc();\
        MEM_BARRIER;\
        uint64_t elapsed_cycle = end - begin; \
        printf("%s: %" PRIu64 " ticks.\n", header_str, elapsed_cycle); \
} while (0)


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


void pin_cpu(size_t core_ID);

int init_mte(int testmte);

unsigned char * mmap_option(int mte, uint64_t size);

unsigned char * mte_tag(unsigned char *ptr, unsigned long long tag, int random);

void create_random_chain(uint64_t* indices, uint64_t len);

void read_write_random_order(uint64_t* indices, uint64_t len, uint64_t** ptr);

void read_read_dependency(uint64_t** ptr, uint64_t count);

void write_random_order(uint64_t* indices, uint64_t* ptr, uint64_t count);

void write_random_order_DSB_ST(uint64_t* indices, uint64_t* ptr, uint64_t count);

void write_random_order_DMB_ST(uint64_t* indices, uint64_t* ptr, uint64_t count);

void read_random_order(uint64_t* indices, uint64_t* ptr, uint64_t count);

void store_load_random_order(uint64_t* indices, uint64_t* ptr, uint64_t count);

void store_load_random_order_DSB_ST(uint64_t* indices, uint64_t* ptr, uint64_t count);

void store_load_random_order_DMB_ST(uint64_t* indices, uint64_t* ptr, uint64_t count);

#endif