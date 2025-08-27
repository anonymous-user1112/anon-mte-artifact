#include "util.h"
#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include <arm_sve.h>


// testmte = 1: Test oob access does not work with MTE. Expect result: crush if MTE is enabled.
// testmte = 2: Test oob access does not work with MTE with SVE instruction.
// testmte = 3: Test memory tag after madvice. Expected result: madvice clears memory tagging.
// testmte = 4: Access tagged memory with no-tag pointer. Expect result: crush if MTE is enabled.
int test_mte(int testmte){

    int page_size = sysconf(_SC_PAGESIZE);

    if((testmte == 1) || (testmte == 2)){

        unsigned char *temp1, *temp2;
        temp1 = mmap_option(1, page_size);
        temp2 = mmap_option(1, page_size);
        printf("temp1: %p\n", temp1);
        printf("temp2: %p\n", temp2);
        for (int i = 0; i < page_size; i++){
            temp1[i] = i % 256;
        }
        for (int i = 0; i < page_size; i++){
            temp2[i] = i % 256;
        }

        unsigned long long curr_key1 = (unsigned long long) (1);
        unsigned long long curr_key2 = (unsigned long long) (2);
        temp1 = mte_tag(temp1, curr_key1, 0);
        temp2 = mte_tag(temp2, curr_key2, 0);
        printf("temp1 after tag: %p\n", temp1);
        printf("temp2 after tag: %p\n", temp2);
        unsigned char *temp1_no_tag = (unsigned char *) ((uint64_t) temp1 & 0x00FFFFFFFFFFFFFF);
        unsigned char *temp2_no_tag = (unsigned char *) ((uint64_t) temp2 & 0x00FFFFFFFFFFFFFF);
        printf("temp1_no_tag: %p\n", temp1_no_tag);
        printf("temp2_no_tag: %p\n", temp2_no_tag);
        uint64_t diff = (uint64_t) temp2_no_tag - (uint64_t) temp1_no_tag;
        printf("diff: %" PRIu64 "\n", diff);
        if(testmte==2){
            svuint8_t va = svld1_u8(svptrue_b8(), &temp2[diff]);
        }else{
            printf("temp1[temp2-temp1]: %d\n", temp1[temp2 - temp1]); // this works
            printf("temp1[diff]: %d\n", temp1[diff]); // this does not work
        }
        
        munmap(temp1, page_size);
        munmap(temp2, page_size);

    }else if((testmte == 3) || (testmte == 4)){

        unsigned char *temp1 = mmap_option(1, page_size);
        printf("temp1: %p\n", temp1);
        unsigned long long curr_key1 = (unsigned long long) (1);
        temp1 = mte_tag(temp1, curr_key1, 0);
        printf("temp1 after tag: %p\n", temp1);
        unsigned char *temp1_no_tag = (unsigned char *) ((uint64_t) temp1 & 0x00FFFFFFFFFFFFFF);
        printf("temp1_no_tag: %p\n", temp1_no_tag);
        if(testmte==3){
            madvise(temp1, page_size, MADV_DONTNEED);
        }
        temp1_no_tag[0] = 100;
        printf("temp1_no_tag[0]: %d\n", temp1_no_tag[0]);
        munmap(temp1, page_size);

    }
    return 0;
            
}


int main(int argc, char *argv[])
{
	
    int testmte = atoi(argv[1]);
    int cpu = atoi(argv[2]);

    printf("testmte %d\n", testmte);
    printf("Pin to CPU %d\n", cpu);

    // PIN to CPU core cpu
    pin_cpu(cpu);

    // Initialize MTE
    init_mte(testmte);

    test_mte(testmte);
    
    return 0;
}