#include "util.h"
#include <stdio.h>
#include <string.h>
#include <inttypes.h>

void perf_no_mte(uint64_t buffer_size){
    
    unsigned char* buffer = NULL;
    unsigned char* indices = NULL;
    
    // Init some variables
    uint64_t** buffer_8;
    uint64_t buffer_size_8 = buffer_size/8;

    buffer = mmap_option(-1, buffer_size);
    mprotect_option(0, buffer, buffer_size);
    indices = mmap_option(-1, buffer_size_8*sizeof(uint64_t));
    mprotect_option(0, indices, buffer_size_8*sizeof(uint64_t));

    buffer_8 = (uint64_t**)buffer;
    create_random_chain((uint64_t*)indices, buffer_size_8);

    read_write_random_order((uint64_t*)indices, buffer_size_8, buffer_8, 1);

    for (uint64_t i = 1; i < buffer_size; i+=64) {
        arm_clean_va_to_poc((void*)(&buffer[i]));
    }

    uint64_t begin_temp = 0;
    uint64_t end_temp = 0;
    
    volatile uint64_t* chase_pointers;
    int curr_count = buffer_size_8;
    uint64_t** p = (uint64_t**) buffer_8;
    uint64_t** old_p = p;
    uint64_t total = 0;

    // MEM_BARRIER;
    // begin_temp = cpucycles();
    // MEM_BARRIER;
    while (curr_count-- > 0) {
        MEM_BARRIER;
        begin_temp = cpucycles();
        MEM_BARRIER;

        p = (uint64_t**) *p;

        MEM_BARRIER;
        end_temp = cpucycles();
        MEM_BARRIER;

        MEM_BARRIER;
        //arm_clean_va_to_poc((void*)old_p);
        MEM_BARRIER;

        old_p = p;
        total += end_temp-begin_temp;
    }
    chase_pointers = *p;

    // read_read_dependency((uint64_t**)buffer_8, buffer_size_8, 1);
    

    // MEM_BARRIER;
    // end_temp = cpucycles();
    // MEM_BARRIER;

    total += end_temp-begin_temp;
    printf("total ticks %" PRIu64 "\n", total);

    munmap(buffer, buffer_size);
    munmap(indices, buffer_size_8*sizeof(uint64_t));

}


void perf_mte(uint64_t buffer_size){

    unsigned char * buffer = NULL;
    unsigned char * indices = NULL;

    uint64_t** buffer_8;
    uint64_t buffer_size_8 = buffer_size/8;

    buffer = mmap_option(-1, buffer_size);
    indices = mmap_option(-1, buffer_size_8*sizeof(uint64_t));
    mprotect_option(1, buffer, buffer_size);
    mprotect_option(0, indices, buffer_size_8*sizeof(uint64_t));

    // Apply tags
    unsigned long long curr_key = (unsigned long long) (1);
    unsigned char *buffer_tag;
    unsigned char *indices_tag = indices;
   
    buffer_tag = mte_tag(buffer, curr_key, 0);
    for (uint64_t j = 16; j < buffer_size; j += 16){
        mte_tag(buffer+j, curr_key, 0);
    }

    buffer_8 = (uint64_t**)buffer_tag;
    create_random_chain((uint64_t*)indices_tag, buffer_size_8);

    read_write_random_order((uint64_t*)indices_tag, buffer_size_8, buffer_8, 1);

    for (uint64_t i = 1; i < buffer_size; i+=64) {
        arm_clean_va_to_poc((void*)(&buffer_tag[i]));
    }

    uint64_t begin_temp = 0;
    uint64_t end_temp = 0;
    uint64_t total = 0;

    volatile uint64_t* chase_pointers;
    int curr_count = buffer_size_8;
    uint64_t** p = (uint64_t**) buffer_tag;
    uint64_t** old_p = p;
    // MEM_BARRIER;
    // begin_temp = cpucycles();
    // MEM_BARRIER;

    // read_read_dependency((uint64_t**)buffer_tag, buffer_size_8, 1);
       

    
    while (curr_count-- > 0) {
        MEM_BARRIER;
        begin_temp = cpucycles();
        MEM_BARRIER;

        p = (uint64_t**) *p;

        MEM_BARRIER;
        end_temp = cpucycles();
        MEM_BARRIER;

        MEM_BARRIER;
        //arm_clean_va_to_poc((void*)old_p);
        MEM_BARRIER;

        old_p = p;
        total += end_temp-begin_temp;
    }
    chase_pointers = *p;
    
    // MEM_BARRIER;
    // end_temp = cpucycles();
    // MEM_BARRIER;

    total += end_temp-begin_temp;
    printf("total ticks %" PRIu64 "\n", total);


    munmap(buffer, buffer_size);
    munmap(indices, buffer_size_8*sizeof(uint64_t));
}

int main(int argc, char *argv[]){
	// 1 mte, 0 no mte, -1 asyn_mte
    int testmte = atoi(argv[1]);
    int size = atoi(argv[2]);
    int cpu = atoi(argv[3]);

    uint64_t buffer_size = size*1024; // KB*size 
    printf("testmte %d\n", testmte);
    printf("buffer_size %" PRIu64 "\n", buffer_size);
    printf("Pin to CPU %d\n", cpu);

    if(buffer_size%16!=0){
        return 0;
    }

    // PIN to CPU core cpu
    pin_cpu(cpu);

    // Initialize MTE
    if(testmte!=0){
        printf("MTE is initialized\n");
        init_mte(testmte);
    }

    // Init cycle counter
    init();
    
    int mte = abs(testmte);

    if(mte == 0){
        perf_no_mte(buffer_size);
    }else{
        printf("MTE\n");
        perf_mte(buffer_size);
    }

    fini();

    return 0;
}