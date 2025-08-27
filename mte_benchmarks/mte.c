/*
 * Memory Tagging Extension (MTE) example for Linux
 *
 * Compile with gcc and use -march=armv8.5-a+memtag
 *    gcc mte-example.c -o mte-example -march=armv8.5-a+memtag
 *
 * Compilation should be done on a recent Arm Linux machine for the .h files to include MTE support.
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/auxv.h>
#include <sys/mman.h>
#include <sys/prctl.h>
#include <inttypes.h>


/*
 * Insert a random logical tag into the given pointer.
 * IRG instruction.
 */
#define insert_random_tag(ptr) ({                       \
        uint64_t __val;                                 \
        asm("irg %0, %1" : "=r" (__val) : "r" (ptr));   \
        __val;                                          \
})

/*
 * Set the allocation tag on the destination address.
 * STG instruction.
 */
#define set_tag(tagged_addr) do {                                      \
        asm volatile("stg %0, [%0]" : : "r" (tagged_addr) : "memory"); \
} while (0)

#define ADD_TAG_TO_POINTER(ptr, input_tag) \
    ((unsigned char*)((uint64_t)(ptr) | ((input_tag) << 56)))


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


unsigned char * mmap_option(int mte){
    unsigned char *ptr;
    if(mte==1){
        ptr = mmap(NULL, sysconf(_SC_PAGESIZE), PROT_READ | PROT_WRITE | PROT_MTE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        if (ptr == MAP_FAILED)
        {
            perror("mmap() failed");
            return EXIT_FAILURE;
        }
    }else{
        ptr = mmap(NULL, sysconf(_SC_PAGESIZE), PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        if (ptr == MAP_FAILED)
        {
            perror("mmap() failed");
            return EXIT_FAILURE;
        }
    }
    return ptr;
}


unsigned char * mte_tag(unsigned char *ptr, unsigned long long tag){
    //unsigned char * ptr_mte = (unsigned char *) insert_random_tag(ptr);
    /*
     * Set the key on the pointer to match the lock on the memory  (STG instruction)
     */
    unsigned char * ptr_mte = ADD_TAG_TO_POINTER(ptr, tag);
    
    set_tag(ptr);
    return ptr_mte;
}

void outofbound(unsigned char* a1, unsigned char* a2) {
    if (a2 > a1) {
        a1[a2 - a1] = 0xff;
    } else if (a1 > a2) {
        a2[a1 - a2] = 0xff;
    } 
}
int main(void)
{
    unsigned char *a1;   // pointer to memory for MTE demonstration
    unsigned char *a2;   
    unsigned char *b1;   
    unsigned char *b2;   
    int index;
    int data;

    init();
    
    /*
     * Allocate 1 page of memory with MTE protection, a1 and a2 do not have MTE, b1 and b2 have MTE
     */
    a1 = mmap_option(0);
    a2 = mmap_option(0);
    b1 = mmap_option(1);
    b2 = mmap_option(1);

    /*
     * Print the pointer value with the default tag (expecting 0)
     */
    printf("a1 is %p\n", a1);
    printf("a2 is %p\n", a2);
    printf("b1 is %p\n", b1);
    printf("b2 is %p\n", b2);

    // MTE b1 and b2
    b1 = mte_tag(b1, 1ULL);
    b2 = mte_tag(b2, 2ULL);
	
    /*
     * Print the pointer value with the new tag
     */
    printf("a1 is now %p\n", a1);
    printf("a2 is now %p\n", a2);
    printf("b1 is %p\n", b1);
    printf("b2 is %p\n", b2);

    // Out of bound test
    printf("a1[a2-a1] is %d\n", a1[a2-a1]);
    outofbound(a1, a2);
    printf("a1[a2-a1] is now %d\n", a1[a2-a1]);
    
    //printf("b1[b2-b1] is %d\n", b1[b2-b1]);
    //outofbound(b1, b2);
    //printf("b1[b2-b1] is now %d\n", b1[b2-b1]);
    
    printf("b1[16] is %d\n", b1[16]);
    
    //free(a1);
    //free(a2);
    //free(b1);
    //free(b2);
    return EXIT_FAILURE;
}
