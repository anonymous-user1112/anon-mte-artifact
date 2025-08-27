#include "util.h"
#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include "LFSR.hpp"
#include "LFSR_util.hpp"



int main(int argc, char *argv[]){
    int size = atoi(argv[1]);
    int cpu = atoi(argv[2]);
    int lfsr_bit = atoi(argv[3]);

    uint64_t buffer_size = size*1024; // KB*size 
    uint64_t buffer_size_total = buffer_size*1;

    printf("buffer_size %" PRIu64 "\n", buffer_size);
    printf("Pin to CPU %d\n", cpu);
    printf("lfsr_bit %d\n", lfsr_bit);

    // PIN to CPU core cpu
    pin_cpu(cpu);

    // Init cycle counter
    init();

    unsigned char* buffer = NULL;
    
    buffer = mmap_option(-1, buffer_size);
    mprotect_option(0, buffer, buffer_size);
    for (int i = 0; i < buffer_size; i++){
        buffer[i] = i % 256;
    }
    
    // LFSR
    LFSR lfsr(lfsr_bit);
    
    // Set the first bit to 1
    lfsr.setBit(0, true);

    // Save the register
    uint32_t * output;
    lfsr.save(output);

    bool (*getBitOperation)(LFSR &);

    switch (lfsr_bit) {
        case 10:
            getBitOperation = xor10;
            break;
        case 11:
            getBitOperation = xor11;
            break;
        case 12:
            getBitOperation = xor12;
            break;
        case 13:
            getBitOperation = xor13;
            break;
        case 14:
            getBitOperation = xor14;
            break;
        case 15:
            getBitOperation = xor15;
            break;
        case 16:
            getBitOperation = xor16;
            break;
        case 17:
            getBitOperation = xor17;
            break;
        case 18:
            getBitOperation = xor18;
            break;
        case 19:
            getBitOperation = xor19;
            break;
        case 20:
            getBitOperation = xor20;
            break;
        case 21:
            getBitOperation = xor21;
            break;
        case 22:
            getBitOperation = xor22;
            break;
        case 23:
            getBitOperation = xor23;
            break;
        case 24:
            getBitOperation = xor24;
            break;
        case 25:
            getBitOperation = xor25;
            break;
        default:
            getBitOperation = xor25;
    }

    for(int i = 0; i<500; i++){
        MEASURE_CYCLE(
            for (int j = 0; j < buffer_size_total; j++){
                bool result = getBitOperation(lfsr);
                lfsr.rightShift(result);
                uint32_t index = lfsr.getArray();  
                index = prng(j) * index;              
                FORCE_READ_INT(buffer[index % buffer_size]);
            }
            , 
            "Pointer Chasing"
        );
    }

    fini();
    munmap(buffer, buffer_size);

    return 0;

    // unsigned char* indices = NULL;
    
    // uint64_t buffer_size_8 = buffer_size/8;
    // indices = mmap_option(-1, buffer_size_8*sizeof(uint64_t));
    // mprotect_option(0, indices, buffer_size_8*sizeof(uint64_t));

    // uint64_t** buffer_8 = (uint64_t**)buffer;
    // create_random_chain((uint64_t*)indices, buffer_size_8);

    // read_write_random_order((uint64_t*)indices, buffer_size_8, buffer_8, 1);

    // for(int i = 0; i<100; i++){

    //     MEASURE_CYCLE(
    //         //read_read_dependency(buffer_8, buffer_size_8, 1);
    //         read_random_order(indices, (uint64_t*)buffer_8, buffer_size_8, 1);
    //         , 
    //         "Pointer Chasing"
    //     );

    // }
    
    // munmap(indices, buffer_size_8*sizeof(uint64_t));
    
}