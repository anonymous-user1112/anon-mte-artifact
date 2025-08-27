#include "util.h"
#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include "LFSR.hpp"
#include "LFSR_util.hpp"

void bench_no_mte(uint64_t buffer_size,  int iteration, int workload_iteration){

    unsigned char * buffer = NULL;
    unsigned char* indices = NULL;
    
    // Init some variables
    uint64_t** buffer_8;
    uint64_t buffer_size_8 = buffer_size/8;

    MEASURE_CYCLE(
        buffer = mmap_option(-1, buffer_size);
        mprotect_option(0, buffer, buffer_size);
        indices = mmap_option(-1, buffer_size_8*sizeof(uint64_t));
        mprotect_option(0, indices, buffer_size_8*sizeof(uint64_t));
        , 
        "Setup"
    );
            
    buffer_8 = (uint64_t**)buffer;
    create_random_chain((uint64_t*)indices, buffer_size_8);
    
    // LFSR
    LFSR lfsr(25);
    // Set the first bit to 1
    lfsr.setBit(0, true);
    // Save the register
    uint32_t * output;
    lfsr.save(output);

    for(int i = 0; i<iteration; i++){

        read_write_random_order((uint64_t*)indices, buffer_size_8, buffer_8, workload_iteration);

        MEASURE_CYCLE(
            read_read_dependency(buffer_8, buffer_size_8, workload_iteration);
            , 
            "Read after read to random memory location with dependency (sequential order): chase pointer"
        );

        MEASURE_CYCLE(
            write_random_order((uint64_t*)buffer_8, buffer_size_8, lfsr, workload_iteration);
            , 
            "Write after write to random memory location no dependency"
        );

        MEASURE_CYCLE(
            read_random_order((uint64_t*)buffer_8, buffer_size_8, lfsr, workload_iteration);
            , 
            "Read after read to random memory location no dependency"
        );
        
        MEASURE_CYCLE(
            store_load_random_order((uint64_t*)buffer_8, buffer_size_8, lfsr, workload_iteration);
            , 
            "store to load forwarding"
        );

        MEASURE_CYCLE(
            read_seq_only((uint64_t*)buffer_8, buffer_size_8, workload_iteration);
            , 
            "Read after read sequential order"
        );

        MEASURE_CYCLE(
            write_seq_only((uint64_t*)buffer_8, buffer_size_8, workload_iteration);
            , 
            "Write after write sequential order"
        );

        MEASURE_CYCLE(
            read_write_seq_only((uint64_t*)buffer_8, buffer_size_8, workload_iteration);
            , 
            "Read after write sequential order"
        );

    }

    munmap(buffer, buffer_size);
    munmap(indices, buffer_size_8*sizeof(uint64_t));
}


void bench_mte(uint64_t buffer_size, int iteration, int workload_iteration){
    
    unsigned char * buffer = NULL;
    unsigned char * indices = NULL;

    uint64_t** buffer_8;
    uint64_t buffer_size_8 = buffer_size/8;

    MEASURE_CYCLE(
        buffer = mmap_option(-1, buffer_size);
        indices = mmap_option(-1, buffer_size_8*sizeof(uint64_t));
        mprotect_option(1, buffer, buffer_size);
        mprotect_option(0, indices, buffer_size_8*sizeof(uint64_t));
        , 
        "Setup with MTE"
    );

    // Apply tags
    unsigned long long curr_key = (unsigned long long) (1);
    unsigned char *buffer_tag;
    unsigned char *indices_tag = indices;
   
    MEASURE_CYCLE(
        buffer_tag = mte_tag(buffer, curr_key, 0);
        for (uint64_t j = 16; j < buffer_size; j += 16){
            mte_tag(buffer+j, curr_key, 0);
        }
        , 
        "Apply MTE Tag"
    );

    buffer_8 = (uint64_t**)buffer_tag;
    create_random_chain((uint64_t*)indices_tag, buffer_size_8);
    
    // LFSR
    LFSR lfsr(25);
    // Set the first bit to 1
    lfsr.setBit(0, true);
    // Save the register
    uint32_t * output;
    lfsr.save(output);

    for(int i = 0; i<iteration; i++){

        read_write_random_order((uint64_t*)indices_tag, buffer_size_8, buffer_8, workload_iteration);

        MEASURE_CYCLE(
            read_read_dependency((uint64_t**)buffer_tag, buffer_size_8, workload_iteration);
            , 
            "MTE: Read after read to random memory location with dependency (sequential order): chase pointer"
        );

        MEASURE_CYCLE(
            write_random_order((uint64_t*)buffer_tag, buffer_size_8, lfsr, workload_iteration);
            , 
            "MTE: Write after write to random memory location no dependency"
        );

        MEASURE_CYCLE(
            write_random_order_ST((uint64_t*)buffer_tag, buffer_size_8, lfsr, workload_iteration);
            , 
            "MTE + DSB ST: Write after write to random memory location no dependency"
        );

        MEASURE_CYCLE(
            write_random_order_DMB_ST((uint64_t*)buffer_tag, buffer_size_8, lfsr, workload_iteration);
            , 
            "MTE + DMB ST: Write after write to random memory location no dependency"
        );

        MEASURE_CYCLE(
            read_random_order((uint64_t*)buffer_tag, buffer_size_8, lfsr, workload_iteration);
            , 
            "MTE: Read after read to random memory location no dependency"
        );

        MEASURE_CYCLE(
            store_load_random_order((uint64_t*)buffer_tag, buffer_size_8, lfsr, workload_iteration);
            , 
            "MTE: store to load forwarding"
        );

        MEASURE_CYCLE(
            store_load_random_order_dsb_any((uint64_t*)buffer_tag, buffer_size_8, lfsr, workload_iteration);
            , 
            "MTE + DSB: store to load forwarding"
        );

        MEASURE_CYCLE(
            store_load_random_order_dmb_any((uint64_t*)buffer_tag, buffer_size_8, lfsr, workload_iteration);
            , 
            "MTE + DMB: store to load forwarding"
        );

        MEASURE_CYCLE(
            read_seq_only((uint64_t*)buffer_8, buffer_size_8, workload_iteration);
            , 
            "MTE: Read after read sequential order"
        );

        MEASURE_CYCLE(
            write_seq_only((uint64_t*)buffer_8, buffer_size_8, workload_iteration);
            , 
            "MTE: Write after write sequential order"
        );

        MEASURE_CYCLE(
            read_write_seq_only((uint64_t*)buffer_8, buffer_size_8, workload_iteration);
            , 
            "MTE: Read after write sequential order"
        );
    }

    munmap(buffer, buffer_size);
    munmap(indices, buffer_size_8*sizeof(uint64_t));
}



int main(int argc, char *argv[]){
	
    // 1 mte, 0 no mte, -1 asyn_mte
    int mte = atoi(argv[1]);
    int size = atoi(argv[2]);
    int iteration = atoi(argv[3]);
    int cpu = atoi(argv[4]);

    uint64_t buffer_size = size*1024; // KB*size 
    printf("mte %d\n", mte);
    printf("buffer_size %" PRIu64 "\n", buffer_size);
    printf("iteration %d\n", iteration);
    printf("Pin to CPU %d\n", cpu);

    if(buffer_size%16!=0){
        return 0;
    }

    // PIN to CPU core cpu
    pin_cpu(cpu);

    // Init cycle counter
    init();

    // Initialize MTE
    init_mte(mte);
    
    if( (mte == 1) || (mte == -1)){ // benchmark mte behavior
        bench_mte(buffer_size, iteration, 1);
    }else if(mte == 0){ // benchmark no mte behavior
        bench_no_mte(buffer_size, iteration, 1);
    }

    fini();

    return 0;
}