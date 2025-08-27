#include "util.h"
#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include <stdlib.h>
#include <sys/resource.h>

typedef uint8_t slf_type;

int main(int argc, char *argv[]){
    
    // uint64_t buffer_size = 40960;
    // uint64_t buffer_size_8 = 40960/8;
    // unsigned char* buffer = mmap_option(0, buffer_size);
    // uint64_t* indices = (uint64_t*)mmap_option(0, buffer_size_8*sizeof(uint64_t));

    // create_random_chain(indices, buffer_size_8, NULL);

    // uint64_t** buffer_8 = (uint64_t**)buffer;
    // for (uint64_t i = 1; i < buffer_size_8; ++i) {
    //     buffer_8[indices[i-1]] = (uint64_t*) &buffer_8[indices[i]];  
    // }
    // buffer_8[indices[buffer_size_8-1]] = (uint64_t*) &buffer_8[indices[0]];

    // for (int i = 0; i < buffer_size_8; i++){
    //     arm_clean_va_to_poc((void*)&buffer_8[i]);
    // }
    
    // pin_cpu(0);
    // setpriority(PRIO_PROCESS, 0, -20);
    // init();

    // volatile uint64_t* chase_pointers_global;
    // for (int i = 0; i < 10; i++){
    //     MEM_BARRIER;
    //     uint64_t begin_temp = cpucycles();
    //     MEM_BARRIER;

    //     int curr_count = buffer_size_8;
    //     uint64_t** p = (uint64_t**) buffer_8;
    //     while (curr_count-- > 0) {
    //         p = (uint64_t**) *p;
    //     }
    //     chase_pointers_global = *p;

    //     MEM_BARRIER;
    //     uint64_t end_temp = cpucycles();
    //     MEM_BARRIER;

    //     printf("Pointer chasing : %zu,  %zu,  %zu ticks \n",
    //                     begin_temp, end_temp, end_temp-begin_temp);
    // }
    

    // 1 mte, 0 no mte, -1 asyn_mte
    int testmte = atoi(argv[1]);
    
    uint64_t buffer_size = 4096*sysconf(_SC_PAGESIZE);
    uint64_t num_cl =  buffer_size/64;
    printf("testmte %d\n", testmte);

    // PIN to CPU core 0
    pin_cpu(0);
    setpriority(PRIO_PROCESS, 0, -20);

    // Initialize MTE
    init_mte(testmte);

    unsigned char * buffer = NULL;
    buffer = mmap_option(-1, buffer_size);
    uint64_t physical_addr = get_physical_addr((uint64_t)buffer);

    while(physical_addr%64!=0){
        munmap(buffer, buffer_size);
        buffer = mmap_option(-1, buffer_size);
        physical_addr = get_physical_addr((uint64_t)buffer);
    }

    int mte = abs(testmte);
    mprotect_option(mte, buffer, buffer_size);
    for (uint64_t i = 0; i < buffer_size; i++){
        buffer[i] = i % 256;
    }

    unsigned long long curr_key = (unsigned long long) (1);
    unsigned char *buffer_tag = buffer;
    if(mte){
        buffer_tag = mte_tag(buffer, curr_key, 0);
        for (uint64_t j = 16; j < buffer_size; j += 16){
            mte_tag(buffer+j, curr_key, 0);
        }
    }

    int num_element = 16/sizeof(slf_type);
    init();

    int i = 9;
    

    uint64_t within_cl = 0;
    uint64_t across_cl = 0;
    uint64_t begin_temp = 0;
    uint64_t end_temp = 0;

    for (size_t n = 0; n < 10; n++){
        
        for (uint64_t j = 0; j < buffer_size; j+=4096){
            slf_type* curr_cl = (slf_type*)(&buffer_tag[j]);
            arm_clean_va_to_poc((void*)curr_cl);
        }

        MEM_BARRIER;
        begin_temp = cpucycles();
        MEM_BARRIER;

        for (uint64_t j = 0; j < buffer_size; j+=4096){

            slf_type* curr_cl = (slf_type*)(&buffer_tag[j]);

            curr_cl[17] = curr_cl[0] + i;
            curr_cl[34] = curr_cl[17] + i;
            curr_cl[51] = curr_cl[34] + i;
            curr_cl[4] = curr_cl[51] + i;
            curr_cl[21] = curr_cl[4] + i;
            curr_cl[38] = curr_cl[21] + i;
            curr_cl[55] = curr_cl[38] + i;
            curr_cl[8] = curr_cl[55] + i;
            curr_cl[25] = curr_cl[8] + i;
            curr_cl[42] = curr_cl[25] + i;
            curr_cl[59] = curr_cl[42] + i;
            curr_cl[12] = curr_cl[59] + i;
            curr_cl[29] = curr_cl[12] + i;
            curr_cl[46] = curr_cl[29] + i;
            curr_cl[63] = curr_cl[46] + i;
            curr_cl[16] = curr_cl[63] + i;
            curr_cl[33] = curr_cl[16] + i;
            curr_cl[50] = curr_cl[33] + i;
            curr_cl[3] = curr_cl[50] + i;
            curr_cl[20] = curr_cl[3] + i;
            curr_cl[37] = curr_cl[20] + i;
            curr_cl[54] = curr_cl[37] + i;
            curr_cl[7] = curr_cl[54] + i;
            curr_cl[24] = curr_cl[7] + i;
            curr_cl[41] = curr_cl[24] + i;
            curr_cl[58] = curr_cl[41] + i;
            curr_cl[11] = curr_cl[58] + i;
            curr_cl[28] = curr_cl[11] + i;
            curr_cl[45] = curr_cl[28] + i;
            curr_cl[62] = curr_cl[45] + i;
            curr_cl[15] = curr_cl[62] + i;
            curr_cl[32] = curr_cl[15] + i;
            curr_cl[49] = curr_cl[32] + i;
            curr_cl[2] = curr_cl[49] + i;
            curr_cl[19] = curr_cl[2] + i;
            curr_cl[36] = curr_cl[19] + i;
            curr_cl[53] = curr_cl[36] + i;
            curr_cl[6] = curr_cl[53] + i;
            curr_cl[23] = curr_cl[6] + i;
            curr_cl[40] = curr_cl[23] + i;
            curr_cl[57] = curr_cl[40] + i;
            curr_cl[10] = curr_cl[57] + i;
            curr_cl[27] = curr_cl[10] + i;
            curr_cl[44] = curr_cl[27] + i;
            curr_cl[61] = curr_cl[44] + i;
            curr_cl[14] = curr_cl[61] + i;
            curr_cl[31] = curr_cl[14] + i;
            curr_cl[48] = curr_cl[31] + i;
            curr_cl[1] = curr_cl[48] + i;
            curr_cl[18] = curr_cl[1] + i;
            curr_cl[35] = curr_cl[18] + i;
            curr_cl[52] = curr_cl[35] + i;
            curr_cl[5] = curr_cl[52] + i;
            curr_cl[22] = curr_cl[5] + i;
            curr_cl[39] = curr_cl[22] + i;
            curr_cl[56] = curr_cl[39] + i;
            curr_cl[9] = curr_cl[56] + i;
            curr_cl[26] = curr_cl[9] + i;
            curr_cl[43] = curr_cl[26] + i;
            curr_cl[60] = curr_cl[43] + i;
            curr_cl[13] = curr_cl[60] + i;
            curr_cl[30] = curr_cl[13] + i;
            curr_cl[47] = curr_cl[30] + i;
            curr_cl[0] = curr_cl[47] + i;
        }

        MEM_BARRIER;
        end_temp = cpucycles();
        MEM_BARRIER;

        across_cl = end_temp-begin_temp;

        for (uint64_t j = 0; j < buffer_size; j+=4096){
            slf_type* curr_cl = (slf_type*)(&buffer_tag[j]);
            arm_clean_va_to_poc((void*)curr_cl);
        }

        MEM_BARRIER;
        begin_temp = cpucycles();
        MEM_BARRIER;

        for (uint64_t j = 0; j < buffer_size; j+=4096){

            slf_type* curr_cl = (slf_type*)(&buffer_tag[j]);
            curr_cl[1] = curr_cl[0] + i;
            curr_cl[2] = curr_cl[1] + i;
            curr_cl[3] = curr_cl[2] + i;
            curr_cl[4] = curr_cl[3] + i;
            curr_cl[5] = curr_cl[4] + i;
            curr_cl[6] = curr_cl[5] + i;
            curr_cl[7] = curr_cl[6] + i;
            curr_cl[8] = curr_cl[7] + i;
            curr_cl[9] = curr_cl[8] + i;
            curr_cl[10] = curr_cl[9] + i;
            curr_cl[11] = curr_cl[10] + i;
            curr_cl[12] = curr_cl[11] + i;
            curr_cl[13] = curr_cl[12] + i;
            curr_cl[14] = curr_cl[13] + i;
            curr_cl[15] = curr_cl[14] + i;
            curr_cl[16] = curr_cl[15] + i;
            curr_cl[17] = curr_cl[16] + i;
            curr_cl[18] = curr_cl[17] + i;
            curr_cl[19] = curr_cl[18] + i;
            curr_cl[20] = curr_cl[19] + i;
            curr_cl[21] = curr_cl[20] + i;
            curr_cl[22] = curr_cl[21] + i;
            curr_cl[23] = curr_cl[22] + i;
            curr_cl[24] = curr_cl[23] + i;
            curr_cl[25] = curr_cl[24] + i;
            curr_cl[26] = curr_cl[25] + i;
            curr_cl[27] = curr_cl[26] + i;
            curr_cl[28] = curr_cl[27] + i;
            curr_cl[29] = curr_cl[28] + i;
            curr_cl[30] = curr_cl[29] + i;
            curr_cl[31] = curr_cl[30] + i;
            curr_cl[32] = curr_cl[31] + i;
            curr_cl[33] = curr_cl[32] + i;
            curr_cl[34] = curr_cl[33] + i;
            curr_cl[35] = curr_cl[34] + i;
            curr_cl[36] = curr_cl[35] + i;
            curr_cl[37] = curr_cl[36] + i;
            curr_cl[38] = curr_cl[37] + i;
            curr_cl[39] = curr_cl[38] + i;
            curr_cl[40] = curr_cl[39] + i;
            curr_cl[41] = curr_cl[40] + i;
            curr_cl[42] = curr_cl[41] + i;
            curr_cl[43] = curr_cl[42] + i;
            curr_cl[44] = curr_cl[43] + i;
            curr_cl[45] = curr_cl[44] + i;
            curr_cl[46] = curr_cl[45] + i;
            curr_cl[47] = curr_cl[46] + i;
            curr_cl[48] = curr_cl[47] + i;
            curr_cl[49] = curr_cl[48] + i;
            curr_cl[50] = curr_cl[49] + i;
            curr_cl[51] = curr_cl[50] + i;
            curr_cl[52] = curr_cl[51] + i;
            curr_cl[53] = curr_cl[52] + i;
            curr_cl[54] = curr_cl[53] + i;
            curr_cl[55] = curr_cl[54] + i;
            curr_cl[56] = curr_cl[55] + i;
            curr_cl[57] = curr_cl[56] + i;
            curr_cl[58] = curr_cl[57] + i;
            curr_cl[59] = curr_cl[58] + i;
            curr_cl[60] = curr_cl[59] + i;
            curr_cl[61] = curr_cl[60] + i;
            curr_cl[62] = curr_cl[61] + i;
            curr_cl[63] = curr_cl[62] + i;
            curr_cl[0] = curr_cl[63] + i;
        }


        MEM_BARRIER;
        end_temp = cpucycles();
        MEM_BARRIER;

        within_cl = end_temp-begin_temp;

        printf("Store to load forwarding within cacheline : %zu ticks \n",
                    (size_t)within_cl);

        printf("Store to load forwarding across cacheline : %zu ticks \n",
                    (size_t)across_cl);
    }

    // printf("Store to load forwarding within cacheline : %zu ticks \n",
    //                 (size_t)within_cl);

    // printf("Store to load forwarding across cacheline : %zu ticks \n",
    //                 (size_t)across_cl);

    fini();
    
    return 0;
}