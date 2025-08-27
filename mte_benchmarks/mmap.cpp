#include <string>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/prctl.h>
#include <sys/auxv.h>
#include <vector>
#include <sched.h>
#include <stdint.h>
#include <inttypes.h>


// For MTE
#define PROT_MTE                 0x20
#define total_key                16

#define set_tag(tagged_addr) do {                                      \
        asm volatile("stg %0, [%0]" : : "r" (tagged_addr) : "memory"); \
} while (0)

// Insert with a user-defined tag
#define insert_my_tag(ptr, input_tag) \
    ((unsigned char*)((uint64_t)(ptr) | ((input_tag) << 56)))

#define insert_random_tag(ptr) ({                       \
        uint64_t __val;                                 \
        asm("irg %0, %1" : "=r" (__val) : "r" (ptr));   \
        __val;                                          \
})



int init(){
    /*
     * Use the architecture dependent information about the processor
     * from getauxval() to check if MTE is available.
     */
    if (!((getauxval(AT_HWCAP2)) & HWCAP2_MTE))
    {
        printf("MTE is not supported\n");
        return EXIT_FAILURE;
    }
    else
    {
        printf("MTE is supported\n");
    }

    /*
     * Enable MTE with synchronous checking
     */
    if (prctl(PR_SET_TAGGED_ADDR_CTRL,
              PR_TAGGED_ADDR_ENABLE | PR_MTE_TCF_SYNC | (0xfffe << PR_MTE_TAG_SHIFT),
              0, 0, 0))
    {
            perror("prctl() failed");
            return EXIT_FAILURE;
    }
    return 0;
}

int main(int argc, char *argv[])
{
	
    int num_sandbox = atoi(argv[1]);
    int sel = atoi(argv[2]);
    int testmte = atoi(argv[3]);

    // PIN CPU
    cpu_set_t set;
    CPU_ZERO(&set);
    CPU_SET(1, &set);
    if (sched_setaffinity(0, sizeof(cpu_set_t), &set) < 0) {
        printf("Set cpu failed\n"); 
        return -1;
    }

    double elapsed_time = 0;
    double elapsed_time_setup = 0;

    size_t size_sb = (size_t) 8 * 1024 * 1024 * 1024;
    size_t use_sb = (size_t) 64 * 1024;

    for (int index = 0; index < 500; index++) {

        struct timeval start, end;
        struct timeval start_setup, end_setup;
        std::vector<unsigned char *> mte_sandboxes;
        
        if (sel < 2) { // Baseline cases

            // Set up phase
            // num_sandbox * size_sb sandbox, with no permission, and no file back up.
            gettimeofday(&start_setup, NULL); 

            unsigned char *sandboxes = (unsigned char *) mmap(NULL, size_sb * (size_t) num_sandbox,
                                                              PROT_NONE,
                                                              MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
            if (sandboxes == MAP_FAILED) {
                printf("Mmap failed\n");
                return -1;
            }

            int ret = 0;
            // Give the top 64KB of each 8GB sandbox R+W permission
            for (size_t i = 0; i < num_sandbox; i++) {
                ret = mprotect(&sandboxes[size_sb * i], use_sb, PROT_READ | PROT_WRITE);
                mte_sandboxes.push_back(&sandboxes[size_sb * i]);
                if (ret != 0) {
                    printf("Mprotect failed\n");
                    return -2;
                }
            }

            gettimeofday(&end_setup, NULL);

            if(index>=450) {
                elapsed_time_setup = elapsed_time_setup + (end_setup.tv_sec - start_setup.tv_sec) *
                        1e6 + (end_setup.tv_usec - start_setup.tv_usec);
            }

            if (testmte == 1) {
                // Test oob access works without MTE
                unsigned char *temp1, *temp2;
                temp1 = mte_sandboxes.at(1);
                temp2 = mte_sandboxes.at(2);
                temp2[0] = 100;
                printf("temp1: %p\n", temp1);
                printf("temp2: %p\n", temp2);
                printf("temp1[temp2-temp1]: %d\n", temp1[temp2 - temp1]);
                munmap(sandboxes, size_sb * (size_t) num_sandbox);
                return 0;
            }

            if (sel == 0) {
                // Destroy the 64 KB region of sandboxes with madvice DONTNEED
                // Measure the performance of this Destroy
                gettimeofday(&start, NULL);
                for (size_t i = 0; i < num_sandbox; i++) {
                    ret = madvise(mte_sandboxes.at(i), use_sb, MADV_DONTNEED);
                    if (ret != 0) {
                        printf("Individual madvise failed\n");
                        return -3;
                    }
                }
                gettimeofday(&end, NULL);
                if(index>=450) {
                    elapsed_time = elapsed_time + (end.tv_sec - start.tv_sec) * 1e6 + (end.tv_usec - start.tv_usec);
                }
            } else {
                // Destroy all sandboxes with madvice DONTNEED
                // Measure the performance of this Destroy
                gettimeofday(&start, NULL);
                ret = madvise(mte_sandboxes.at(0), size_sb * (size_t) num_sandbox, MADV_DONTNEED);
                if (ret != 0) {
                    printf("madvise failed\n");
                    return -4;
                }
                gettimeofday(&end, NULL);
                if(index>=450) {
                    elapsed_time = elapsed_time + (end.tv_sec - start.tv_sec) * 1e6 + (end.tv_usec - start.tv_usec);
                }
            }
            munmap(sandboxes, size_sb * (size_t) num_sandbox);

        } else { // MTE cases

            init();

            gettimeofday(&start_setup, NULL);

            // Set up phase
            // num_sandbox * num_sandbox sandbox, with no permission, and no file back up.
            char *sandboxes = (char *) mmap(NULL, size_sb * num_sandbox, PROT_NONE,
                                            MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
            if (sandboxes == MAP_FAILED) {
                printf("mmap failed\n");
                return -7;
            }
            int ret = 0;

            // Give the top 64KB of each 512MB sandbox R+W permission + and MTE key
            for (size_t i = 0; i < num_sandbox; i++) {
                ret = mprotect(&sandboxes[size_sb * i], use_sb, PROT_READ | PROT_WRITE | PROT_MTE);
                if (ret != 0) {
                    printf("mprotect failed\n");
                    return -8;
                }
            }
            for (size_t i = 0; i < num_sandbox; i++) {
                unsigned long long curr_key = (unsigned long long) ((i % (total_key - 1)) + 1);
                unsigned char *temp = insert_my_tag(&sandboxes[size_sb * i], curr_key);
                for (size_t j = 0; j < use_sb; j += (size_t) 16) {
                    if (j == 0) {
                        mte_sandboxes.push_back(temp);
                    }
                    set_tag((unsigned char *) temp);
                    temp = temp + 16;
                }
            }

            gettimeofday(&end_setup, NULL);

            if(index>=450) {
                elapsed_time_setup = elapsed_time_setup + (end_setup.tv_sec - start_setup.tv_sec) *
                                                          1e6 + (end_setup.tv_usec - start_setup.tv_usec);
            }

            if (testmte == 1) {
                // Test oob access does not work with MTE
                unsigned char *temp1, *temp2;
                temp1 = mte_sandboxes.at(1);
                temp2 = mte_sandboxes.at(2);
                temp2[0] = 0xff;
                printf("temp1: %p\n", temp1);
                printf("temp2: %p\n", temp2);
                unsigned char *temp1_no_tag = (unsigned char *) ((uint64_t) temp1 &
                                                                 0x00FFFFFFFFFFFFFF);
                unsigned char *temp2_no_tag = (unsigned char *) ((uint64_t) temp2 &
                                                                 0x00FFFFFFFFFFFFFF);
                printf("temp1_no_tag: %p\n", temp1_no_tag);
                printf("temp2_no_tag: %p\n", temp2_no_tag);
                uint64_t diff = (uint64_t) temp2_no_tag - (uint64_t) temp1_no_tag;
                printf("diff: %" PRIu64 "\n", diff);
                printf("temp1[temp2-temp1]: %d\n", temp1[temp2 - temp1]); // this works
                printf("temp1[diff]: %d\n", temp1[diff]); // this does not work
                munmap(sandboxes, size_sb * (size_t) num_sandbox);
                return 0;
            } else if (testmte == 2) {
                // Test memory tag after madvice
                // Expected result: madvice clears memory tagging
                unsigned char *temp1 = mte_sandboxes.at(0);
                printf("temp1: %p\n", temp1);
                madvise(temp1, use_sb, MADV_DONTNEED);
                unsigned char *temp1_no_tag = (unsigned char *) ((uint64_t) temp1 &
                                                                 0x00FFFFFFFFFFFFFF);
                printf("temp1_no_tag: %p\n", temp1_no_tag);
                temp1_no_tag[0] = 100;
                printf("temp1_no_tag[0]: %d\n", temp1_no_tag[0]);
                munmap(sandboxes, size_sb * (size_t) num_sandbox);
                return 0;
            }

            if (sel == 2) {
                // Destroy the 64 KB region of sandboxes with madvice DONTNEED
                // Measure the performance of this Destroy
                gettimeofday(&start, NULL);
                for (size_t i = 0; i < num_sandbox; i++) {
                    ret = madvise(mte_sandboxes.at(i), use_sb, MADV_DONTNEED);
                    if (ret != 0) {
                        printf("Individual madvise failed \n");
                        return -3;
                    }
                }
                gettimeofday(&end, NULL);
                if(index>=450) {
                    elapsed_time = elapsed_time + (end.tv_sec - start.tv_sec) * 1e6 + (end.tv_usec - start.tv_usec);
                }
            } else {
                // Destroy all sandboxes with madvice DONTNEED
                // Measure the performance of this Destroy
                gettimeofday(&start, NULL);
                ret = madvise(mte_sandboxes.at(0), size_sb * (size_t) num_sandbox, MADV_DONTNEED);
                if (ret != 0) {
                    printf("madvise failed\n");
                    return -4;
                }
                gettimeofday(&end, NULL);
                if(index>=450) {
                    elapsed_time = elapsed_time + (end.tv_sec - start.tv_sec) * 1e6 + (end.tv_usec - start.tv_usec);
                }
            }
            munmap(sandboxes, size_sb * (size_t) num_sandbox);
        }
    }

    printf( "Selector %d, elapsed time setup %.3f, elapsed time exit %.3f \n", sel, elapsed_time_setup/50, elapsed_time/50);
 	
    return 0;
}
