#include "util.h"
#include "sha256.h"
#include <stdio.h>
#include <string.h>
#include <inttypes.h>

// testmte = 1: Test oob access does not work with MTE. Expect result: crush if MTE is enabled.
// testmte = 2: Test memory tag after madvice. Expected result: madvice clears memory tagging.
// testmte = 3: Access tagged memory with no-tag pointer. Expect result: crush if MTE is enabled.
int test_mte(int testmte){

    int page_size = sysconf(_SC_PAGESIZE);

    if(testmte == 1){

        unsigned char *temp1, *temp2;
        temp1 = mmap_option(1, page_size);
        temp2 = mmap_option(1, page_size);
        printf("temp1: %p\n", temp1);
        printf("temp2: %p\n", temp2);
        unsigned long long curr_key1 = (unsigned long long) (0);
        unsigned long long curr_key2 = (unsigned long long) (0);
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
        printf("temp1[temp2-temp1]: %d\n", temp1[temp2 - temp1]); // this works
        printf("temp1[diff]: %d\n", temp1[diff]); // this does not work
        munmap(temp1, page_size);
        munmap(temp2, page_size);

    }else if((testmte == 2) || (testmte == 3)){

        unsigned char *temp1 = mmap_option(1, page_size);
        printf("temp1: %p\n", temp1);
        unsigned long long curr_key1 = (unsigned long long) (1);
        temp1 = mte_tag(temp1, curr_key1, 0);
        printf("temp1 after tag: %p\n", temp1);
        unsigned char *temp1_no_tag = (unsigned char *) ((uint64_t) temp1 & 0x00FFFFFFFFFFFFFF);
        printf("temp1_no_tag: %p\n", temp1_no_tag);
        if(testmte==2){
            madvise(temp1, page_size, MADV_DONTNEED);
        }
        temp1_no_tag[0] = 100;
        printf("temp1_no_tag[0]: %d\n", temp1_no_tag[0]);
        munmap(temp1, page_size);
    }
    return 0;
            
}

void bench_no_mte(uint64_t buffer_size, int workload, int setup_teardown, int iteration, int workload_iteration){
    double elapsed_time = 0;
    struct timeval start, end;

    unsigned char * buffer = NULL;
    unsigned char * ctx = NULL;
    unsigned char* digest = NULL;
    unsigned char* indices = NULL;
    
    // Init some variables
    uint64_t** buffer_8;
    uint64_t buffer_size_8 = buffer_size/8;

    MEASURE_TIME(
        buffer = mmap_option(-1, buffer_size);
        mprotect_option(0, buffer, buffer_size);
        ctx = mmap_option(-1, sizeof(SHA256_CTX));
        mprotect_option(0, ctx, sizeof(SHA256_CTX));
        digest = mmap_option(-1, SHA256_BLOCK_SIZE);
        mprotect_option(0, digest, SHA256_BLOCK_SIZE);
        indices = mmap_option(-1, buffer_size_8*sizeof(uint64_t));
        mprotect_option(0, indices, buffer_size_8*sizeof(uint64_t));
        , 
        "Setup"
    );

    if(setup_teardown==1){

        MEASURE_TIME(
            int ret = madvise(buffer, buffer_size, MADV_DONTNEED);
            ret |= madvise(ctx, sizeof(SHA256_CTX), MADV_DONTNEED);
            ret |= madvise(digest, SHA256_BLOCK_SIZE, MADV_DONTNEED);
            ret |= madvise(indices, buffer_size_8*sizeof(uint64_t), MADV_DONTNEED);
            if (ret != 0) {
                perror("madvise failed");
                exit(EXIT_FAILURE);
            }
            ,
            "Tear down"
        );

    }else{

        for (int i = 0; i < iteration; i++){

            if(workload==0){
            
                buffer_8 = (uint64_t**)buffer;
                create_random_chain((uint64_t*)indices, buffer_size_8);

                MEASURE_TIME(
                    // Fill buffer with pointers chasing
                    read_write_random_order((uint64_t*)indices, buffer_size_8, buffer_8, workload_iteration);
                    , 
                    "Fill buffer with pointer chasing (another version random memory store)"
                );
                    
                MEASURE_TIME(
                    read_read_dependency(buffer_8, buffer_size_8, workload_iteration);
                    , 
                    "Read after read to random memory location with dependency (sequential order): chase pointer with MTE"
                );

                MEASURE_TIME(
                    write_random_order((uint64_t*)indices, (uint64_t*)buffer_8, buffer_size_8, workload_iteration);
                    , 
                    "Write after write to random memory location no dependency"
                );

                MEASURE_TIME(
                    read_random_order((uint64_t*)indices, (uint64_t*)buffer_8, buffer_size_8, workload_iteration);
                    , 
                    "Read after read to random memory location no dependency"
                );
                
                MEASURE_TIME(
                    store_load_random_order((uint64_t*)indices, (uint64_t*)buffer_8, buffer_size_8, workload_iteration);
                    , 
                    "store to load forwarding"
                );

                MEASURE_TIME(
                    write_read_random_order((uint64_t*)indices, (uint64_t*)buffer_8, buffer_size_8, workload_iteration);
                    , 
                    "Write after read to random memory location with dependency"
                );

                MEASURE_TIME(
                    read_seq_only((uint64_t*)buffer_8, buffer_size_8, workload_iteration);
                    , 
                    "Read after read sequential order"
                );

                MEASURE_TIME(
                    write_seq_only((uint64_t*)buffer_8, buffer_size_8, workload_iteration);
                    , 
                    "Write after write sequential order"
                );

                MEASURE_TIME(
                    read_write_seq_only((uint64_t*)buffer_8, buffer_size_8, workload_iteration);
                    , 
                    "Read after write sequential order"
                );

            }else if(workload==1){
                MEASURE_TIME(
                    memset(buffer, 1, buffer_size);
                    sha256_ctx((SHA256_CTX*)ctx, buffer, buffer_size, digest, workload_iteration);
                    , 
                    "Memset and Hash"
                );
            }

            MEASURE_TIME(
                int ret = madvise(buffer, buffer_size, MADV_DONTNEED);
                ret |= madvise(ctx, sizeof(SHA256_CTX), MADV_DONTNEED);
                ret |= madvise(digest, SHA256_BLOCK_SIZE, MADV_DONTNEED);
                ret |= madvise(indices, buffer_size_8*sizeof(uint64_t), MADV_DONTNEED);
                if (ret != 0) {
                    perror("madvise failed");
                    exit(EXIT_FAILURE);
                }
                ,
                "Tear down"
            );
        }

    }

    munmap(buffer, buffer_size);
    munmap(ctx, sizeof(SHA256_CTX));
    munmap(digest, SHA256_BLOCK_SIZE);
    munmap(indices, buffer_size_8*sizeof(uint64_t));
}



void bench_mte(uint64_t buffer_size, int workload, int setup_teardown, int iteration, int workload_iteration){
    double elapsed_time = 0;
    struct timeval start, end;

    unsigned char * buffer = NULL;
    unsigned char * indices = NULL;
    unsigned char * ctx = NULL;
    unsigned char* digest = NULL;

    uint64_t** buffer_8;
    uint64_t buffer_size_8 = buffer_size/8;

    MEASURE_TIME(
        buffer = mmap_option(-1, buffer_size);
        ctx = mmap_option(-1, sizeof(SHA256_CTX));
        digest = mmap_option(-1, SHA256_BLOCK_SIZE);
        indices = mmap_option(-1, buffer_size_8*sizeof(uint64_t));
        mprotect_option(1, buffer, buffer_size);
        mprotect_option(1, ctx, sizeof(SHA256_CTX));
        mprotect_option(1, digest, SHA256_BLOCK_SIZE);
        mprotect_option(0, indices, buffer_size_8*sizeof(uint64_t));
        , 
        "Setup with MTE"
    );

    // Apply tags
    unsigned long long curr_key = (unsigned long long) (1);
    unsigned char *buffer_tag;

    unsigned char *ctx_tag = ctx;
    unsigned char *digest_tag = digest;
    unsigned char *indices_tag = indices;

    if(setup_teardown==1){
        
        MEASURE_TIME(
            buffer_tag = mte_tag(buffer, curr_key, 0);
            for (uint64_t j = 16; j < buffer_size; j += 16){
                mte_tag(buffer+j, curr_key, 0);
            }
            ctx_tag = mte_tag(ctx, curr_key, 0);
            for (uint64_t j = 16; j < sizeof(SHA256_CTX); j += 16){
                mte_tag(ctx+j, curr_key, 0);
            }
            digest_tag = mte_tag(digest, curr_key, 0);
            for (uint64_t j = 16; j < SHA256_BLOCK_SIZE; j += 16){
                mte_tag(digest+j, curr_key, 0);
            }        
            , 
            "Apply MTE Tag"
        );

        MEASURE_TIME(
            int ret = madvise(buffer_tag, buffer_size, MADV_DONTNEED);
            ret |= madvise(ctx_tag, sizeof(SHA256_CTX), MADV_DONTNEED);
            ret |= madvise(digest_tag, SHA256_BLOCK_SIZE, MADV_DONTNEED);
            ret |= madvise(indices_tag, buffer_size_8*sizeof(uint64_t), MADV_DONTNEED);
            if (ret != 0) {
                perror("madvise failed");
                exit(EXIT_FAILURE);
            }
            ,
            "Tear down with MTE"
        );

    }else{

        for (int i = 0; i < iteration; i++){
            
            MEASURE_TIME(
                buffer_tag = mte_tag(buffer, curr_key, 0);
                for (uint64_t j = 16; j < buffer_size; j += 16){
                    mte_tag(buffer+j, curr_key, 0);
                }
                ctx_tag = mte_tag(ctx, curr_key, 0);
                for (uint64_t j = 16; j < sizeof(SHA256_CTX); j += 16){
                    mte_tag(ctx+j, curr_key, 0);
                }
                digest_tag = mte_tag(digest, curr_key, 0);
                for (uint64_t j = 16; j < SHA256_BLOCK_SIZE; j += 16){
                    mte_tag(digest+j, curr_key, 0);
                }
                , 
                "Apply MTE Tag"
            );

            if( workload==0 ){
                buffer_8 = (uint64_t**)buffer_tag;
                create_random_chain((uint64_t*)indices_tag, buffer_size_8);

                MEASURE_TIME(
                    // Fill buffer with pointers chasing
                    read_write_random_order((uint64_t*)indices_tag, buffer_size_8, buffer_8, workload_iteration);
                    , 
                    "MTE: Fill buffer with pointer chasing (another version random memory store)"
                );

                MEASURE_TIME(
                    read_read_dependency((uint64_t**)buffer_tag, buffer_size_8, workload_iteration);
                    , 
                    "MTE: Read after read to random memory location with dependency (sequential order): chase pointer with MTE"
                );

                MEASURE_TIME(
                    write_random_order((uint64_t*)indices_tag, (uint64_t*)buffer_tag, buffer_size_8, workload_iteration);
                    , 
                    "MTE: Write after write to random memory location no dependency"
                );

                MEASURE_TIME(
                    read_random_order((uint64_t*)indices_tag, (uint64_t*)buffer_tag, buffer_size_8, workload_iteration);
                    , 
                    "MTE: Read after read to random memory location no dependency"
                );

                MEASURE_TIME(
                    store_load_random_order((uint64_t*)indices_tag, (uint64_t*)buffer_tag, buffer_size_8, workload_iteration);
                    , 
                    "MTE: store to load forwarding"
                );

                MEASURE_TIME(
                    write_read_random_order((uint64_t*)indices_tag, (uint64_t*)buffer_tag, buffer_size_8, workload_iteration);
                    , 
                    "MTE: Write after read to random memory location with dependency"
                );

                MEASURE_TIME(
                    read_seq_only((uint64_t*)buffer_8, buffer_size_8, workload_iteration);
                    , 
                    "MTE: Read after read sequential order"
                );

                MEASURE_TIME(
                    write_seq_only((uint64_t*)buffer_8, buffer_size_8, workload_iteration);
                    , 
                    "MTE: Write after write sequential order"
                );

                MEASURE_TIME(
                    read_write_seq_only((uint64_t*)buffer_8, buffer_size_8, workload_iteration);
                    , 
                    "MTE: Read after write sequential order"
                );
            }else if(workload==1){
                MEASURE_TIME(
                    memset(buffer_tag, 1, buffer_size);
                    sha256_ctx((SHA256_CTX*)ctx_tag, buffer_tag, buffer_size, digest_tag, workload_iteration);
                    , 
                    "MTE: Memset and Hash"
                );
            }

            MEASURE_TIME(
                int ret = madvise(buffer_tag, buffer_size, MADV_DONTNEED);
                ret |= madvise((SHA256_CTX*)ctx_tag, sizeof(SHA256_CTX), MADV_DONTNEED);
                ret |= madvise(digest_tag, SHA256_BLOCK_SIZE, MADV_DONTNEED);
                ret |= madvise(indices_tag, buffer_size_8*sizeof(uint64_t), MADV_DONTNEED);
                if (ret != 0) {
                    perror("madvise failed");
                    exit(EXIT_FAILURE);
                }
                ,
                "Tear down with MTE"
            );
        }
    }

    munmap(buffer, buffer_size);
    munmap(ctx, sizeof(SHA256_CTX));
    munmap(digest, SHA256_BLOCK_SIZE);
    munmap(indices, buffer_size_8*sizeof(uint64_t));

}

int main(int argc, char *argv[])
{
	
    int testmte = atoi(argv[1]);
    int size = atoi(argv[2]);
    int setup_teardown = atoi(argv[3]);
    int workload = atoi(argv[4]);
    int workload_iteration = atoi(argv[5]);
    int iteration = atoi(argv[6]);
    int cpu = atoi(argv[7]);

    uint64_t buffer_size = size*1024; // KB*size 
    printf("testmte %d\n", testmte);
    printf("buffer_size %" PRIu64 "\n", buffer_size);
    printf("Test setup_teardown only %d\n", setup_teardown);
    printf("workload %d\n", workload);
    printf("Inner workload_iteration %d\n", workload_iteration);
    printf("Outer iteration %d\n", iteration);
    printf("Pin to CPU %d\n", cpu);

    if(buffer_size%16!=0){
        return 0;
    }

    // PIN to CPU core cpu
    pin_cpu(cpu);

    // Initialize MTE
    init_mte(testmte);

    if(testmte > 0){ // we want to test MTE works without benchmark anything
        test_mte(testmte);
        return 0;
    }else if( (testmte == 0) || (testmte == -1)){ // benchmark mte behavior
        if(setup_teardown==1){
            for (int i = 0; i < iteration; i++){
                bench_mte(buffer_size, workload, setup_teardown, iteration, workload_iteration);
            }
        }else{
            bench_mte(buffer_size, workload, setup_teardown, iteration, workload_iteration);
        }
        
    }else if(testmte == -2){ // benchmark no mte behavior
        if(setup_teardown==1){
            for (int i = 0; i < iteration; i++){
                bench_no_mte(buffer_size, workload, setup_teardown, iteration, workload_iteration);
            }
        }else{
            bench_no_mte(buffer_size, workload, setup_teardown, iteration, workload_iteration);
        }
    }

    return 0;
}