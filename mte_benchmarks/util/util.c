#include "util.h"

volatile uint64_t* chase_pointers_global; // to defeat optimizations for pointer chasing algorithm
volatile uint64_t total; 
SHA256_CTX ctx; // for sha256

void pin_cpu(size_t core_ID)
{
	cpu_set_t set;
	CPU_ZERO(&set);
	CPU_SET(core_ID, &set);
	if (sched_setaffinity(0, sizeof(cpu_set_t), &set) < 0) {
		printf("Unable to Set Affinity\n");
		exit(EXIT_FAILURE);
	}
}


int init_mte(int testmte){
    /*
     * Use the architecture dependent information about the processor
     * from getauxval() to check if MTE is available.
     */
    if (!((getauxval(AT_HWCAP2)) & HWCAP2_MTE)){
        printf("MTE is not supported\n");
        return EXIT_FAILURE;
    }else{
        printf("MTE is supported\n");
    }

    /*
     * Enable MTE with synchronous checking
     */
    int MTE = PR_MTE_TCF_SYNC;
    if(testmte==-1){
        MTE = PR_MTE_TCF_ASYNC;
    }
    if (prctl(PR_SET_TAGGED_ADDR_CTRL,
              PR_TAGGED_ADDR_ENABLE | MTE | (0xfffe << PR_MTE_TAG_SHIFT),
              0, 0, 0)){
            perror("prctl() failed");
            return EXIT_FAILURE;
    }
    return 0;
}


unsigned char * mmap_option(int mte, uint64_t size){
    unsigned char *ptr;
    if(mte==1){
        ptr = mmap(NULL, size, PROT_READ | PROT_WRITE | PROT_MTE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        if (ptr == MAP_FAILED){
            perror("mmap() failed");
            exit(EXIT_FAILURE);
        }
    }else if(mte==0){
        ptr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        if (ptr == MAP_FAILED){
            perror("mmap() failed");
            exit(EXIT_FAILURE);
        }
    }else if(mte==-1){
        ptr = mmap(NULL, size, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        if (ptr == MAP_FAILED){
            perror("mmap() failed");
            exit(EXIT_FAILURE);
        }
    }
    return ptr;
}


void mprotect_option(int mte, unsigned char * ptr, uint64_t size){
    int ret = 0;
    if(mte==1){
        ret = mprotect(ptr, size, PROT_READ | PROT_WRITE | PROT_MTE);
        if (ret != 0) {
            perror("mprotect failed");
            exit(EXIT_FAILURE);
        }
    }else if(mte==0){
        ret = mprotect(ptr, size, PROT_READ | PROT_WRITE);
        if (ret != 0) {
            perror("mprotect failed");
            exit(EXIT_FAILURE);
        }
    }
}

unsigned char * mte_tag(unsigned char *ptr, unsigned long long tag, int random){
    unsigned char * ptr_mte = NULL;
    if(random==1){
        ptr_mte = (unsigned char *) insert_random_tag(ptr);
    }else{
        ptr_mte = insert_my_tag(ptr, tag);
    }

    set_tag(ptr_mte);
    return ptr_mte;
}


/* create a cyclic pointer chain that covers all words
   in a memory section of the given size in a randomized order */
// Inspired by: https://github.com/afborchert/pointer-chasing/blob/master/random-chase.cpp
void create_random_chain(uint64_t* indices, uint64_t len) {
    // shuffle indices
    for (uint64_t i = 0; i < len; ++i) {
        indices[i] = i;
    }
    for (uint64_t i = 0; i < len-1; ++i) {
        uint64_t j = i + rand()%(len - i);
        if (i != j) {
            uint64_t temp = indices[i];
            indices[i] = indices[j];
            indices[j] = temp;
        }
    }
    total = 0;
}

void read_write_random_order(uint64_t* indices, uint64_t len, uint64_t** ptr, int workload_iter) {
    // fill memory with pointer references
    for(int j = 0; j<workload_iter; j++){
        for (uint64_t i = 1; i < len; ++i) {
            ptr[indices[i-1]] = (uint64_t*) &ptr[indices[i]];  
        }
        ptr[indices[len-1]] = (uint64_t*) &ptr[indices[0]];
    }
}

// read after read: dependency
void read_read_dependency(uint64_t** ptr, uint64_t count, int workload_iter) {
    // chase the pointers count times
    for(int j = 0; j<workload_iter; j++){
        int curr_count = count;
        uint64_t** p = (uint64_t**) ptr;
        while (curr_count-- > 0) {
            p = (uint64_t**) *p;
        }
        chase_pointers_global = *p;
    }
}

// write after write: no dependency
void write_random_order(uint64_t* indices, uint64_t* ptr, uint64_t count, int workload_iter) {
    for(int j = 0; j<workload_iter; j++){
        for (uint64_t i = 0; i < count; ++i) {
            ptr[indices[i]] = (uint64_t)i+j;  
        }
    }
}

// read after read: no dependency
void read_random_order(uint64_t* indices, uint64_t* ptr, uint64_t count, int workload_iter) {
    for(int j = 0; j<workload_iter; j++){
        for (uint64_t i = 0; i < count; ++i) {
            int foo = ptr[indices[i]] ;
            FORCE_READ_INT(foo);
        }
    }
}

// read after write: dependency
void store_load_random_order(uint64_t* indices, uint64_t* ptr, uint64_t count, int workload_iter) {
    for(int j = 0; j<workload_iter; j++){
        for (uint64_t i = 0; i < count-1; i++) {
            ptr[indices[i+1]] = ptr[indices[i]] + i + j;
        }
    }
}


// write after read: dependency
void write_read_random_order(uint64_t* indices, uint64_t* ptr, uint64_t count, int workload_iter) {
    for(int j = 0; j<workload_iter; j++){
        for (uint64_t i = 1; i < count; ++i) {
            ptr[indices[i-1]] = ptr[indices[i]]+(uint64_t)i+j;  
        }
        //ptr[indices[count-1]] = ptr[indices[count-1]]+(uint64_t)count-1+j;
    }
}


void sha256_ctx(SHA256_CTX * ctx, const unsigned char * str, uint64_t size, unsigned char hash[], int workload_iter){
    for(int j = 0; j<workload_iter; j++){
        sha256_init(ctx);
        sha256_update(ctx, str, size);
        sha256_final(ctx, hash);
    }
}

// read after read sequential
void read_seq_only(uint64_t* ptr, uint64_t size, int workload_iter){
    for(int j = 0; j<workload_iter; j++){
        total = 0;
        for(int i=0; i<size; i++){
            total = total + ptr[i];
        }
    }
}

// write after write sequential
void write_seq_only(uint64_t* ptr, uint64_t size, int workload_iter){
    for(int j = 0; j<workload_iter; j++){
        for(int i=0; i<size; i++){
            ptr[i] = i+j;
        }
    }
}

// read after write sequential
void read_write_seq_only(uint64_t* ptr, uint64_t size, int workload_iter){
    for(int j = 0; j<workload_iter; j++){
        for(int i=0; i<size-1; i++){
            ptr[i+1] = ptr[i] + i + j;
        }
    }
}

// get_physical_addr function from https://github.com/IAIK/flipfloyd
uint64_t get_physical_addr(uint64_t virtual_addr){
	int g_pagemap_fd = -1;
	uint64_t value;

	// open the pagemap
	if(g_pagemap_fd == -1) {
	  g_pagemap_fd = open("/proc/self/pagemap", O_RDONLY);
	}
	if(g_pagemap_fd == -1) return 0;

	// read physical address
	off_t offset = (virtual_addr / 4096) * sizeof(value);
	int got = pread(g_pagemap_fd, &value, sizeof(value), offset);
	if(got != 8) return 0;

	// Check the "page present" flag.
	if(!(value & (1ULL << 63))) return 0;

	// return physical address
	uint64_t frame_num = value & ((1ULL << 55) - 1);
	return (frame_num * 4096) | (virtual_addr & (4095));
}

static int fddev = -1;
void init(void)
{
	static struct perf_event_attr attr;
	attr.type = PERF_TYPE_HARDWARE;
	attr.config = PERF_COUNT_HW_CPU_CYCLES;
	fddev = syscall(__NR_perf_event_open, &attr, 0, -1, -1, 0);
}

void fini(void)
{
	close(fddev);
}

long long
cpucycles(void)
{
	long long result = 0;
	if (read(fddev, &result, sizeof(result)) < sizeof(result)) return 0;
	return result;
}
