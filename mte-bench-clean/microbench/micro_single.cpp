#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include <stdlib.h>  
#include <sched.h>
#include "util.hpp"

void bench_no_mte(uint64_t buffer_size,  int iteration, int workload){
    unsigned char * buffer = NULL;
    uint64_t* indices = NULL;
    uint64_t indices_size = buffer_size/8;

    buffer = mmap_option(0, buffer_size);
    indices = (uint64_t*)mmap_option(0, indices_size*sizeof(uint64_t));
    create_random_chain((uint64_t*)indices, indices_size);

    uint64_t** buffer_ptr = (uint64_t**)buffer;

    read_write_random_order(indices, indices_size, buffer_ptr);

    if(workload==1){ // rar
        for(int i = 0; i<iteration; i++){
            read_read_dependency(buffer_ptr, indices_size);
        }
    }else if(workload == 2){ // waw
        for(int i = 0; i<iteration; i++){
            write_random_order(indices, (uint64_t*)buffer_ptr, indices_size);
        }
    }else if(workload == 3){ // stl
        for(int i = 0; i<iteration; i++){
            store_load_random_order(indices, (uint64_t*)buffer_ptr, indices_size);
        }
    }

    munmap(buffer, buffer_size);
    munmap(indices, indices_size*sizeof(uint64_t));

}


void bench_mte(uint64_t buffer_size, int iteration, int workload){
    unsigned char * buffer = NULL;
    uint64_t* indices = NULL;
    uint64_t indices_size = buffer_size/8;

    buffer = mmap_option(1, buffer_size);
    indices = (uint64_t*)mmap_option(0, indices_size*sizeof(uint64_t));
    create_random_chain((uint64_t*)indices, indices_size);

    unsigned long long curr_key = (unsigned long long) (1);
    unsigned char *buffer_tag;
    buffer_tag = mte_tag(buffer, curr_key, 0);
    for (uint64_t j = 16; j < buffer_size; j += 16){
        mte_tag(buffer+j, curr_key, 0);
    }
    uint64_t** buffer_ptr = (uint64_t**)buffer_tag;

    read_write_random_order(indices, indices_size, buffer_ptr);

    if(workload==1){ // rar
        for(int i = 0; i<iteration; i++){
            read_read_dependency(buffer_ptr, indices_size);
        }
    }else if(workload == 2){ // waw
        for(int i = 0; i<iteration; i++){
            write_random_order(indices, (uint64_t*)buffer_ptr, indices_size);
        }
    }else if(workload == 3){ // stl
        for(int i = 0; i<iteration; i++){
            store_load_random_order(indices, (uint64_t*)buffer_ptr, indices_size);
        }
    }

    
    munmap(buffer, buffer_size);
    munmap(indices, indices_size*sizeof(uint64_t));

}


int main(int argc, char *argv[]){
    if (argc !=6)
		exit(1);

    // 1 mte, 0 no mte, -1 asyn_mte
    int mte = atoi(argv[1]);
    int size = atoi(argv[2]);
    int iteration = atoi(argv[3]);
    int cpu = atoi(argv[4]);
    int workload = atoi(argv[5]);

    if(size%4096!=0){
        exit(1);
    }

    printf("%u, %u, %u, %u, %u\n", mte, size, iteration, cpu, workload);

    pin_cpu((size_t)cpu);

    // Initialize MTE
    if(mte!=0){
        init_mte(mte);
    }
    if( (mte == 1) || (mte == -1)){ // benchmark mte behavior
        bench_mte(size, iteration, workload);
    }else if(mte == 0){ // benchmark no mte behavior
        bench_no_mte(size, iteration, workload);
    }

    return 0;
}