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

int main(void)
{
    unsigned char *ptr;   // pointer to memory for MTE demonstration

    /*
     * Use the architecture dependent information about the processor
     * from getauxval() to check if MTE is available.
     */
    if (!((getauxval(AT_HWCAP2)) & HWCAP2_MTE))
    {
        printf("MTE is not supported. Continue [y/n] : ");

        char response;
        do {
            scanf(" %c", &response);
            if (response == 'y' || response == 'Y') {
                break;
            } else if (response == 'n' || response == 'N') {
                return EXIT_FAILURE;
            } else {
                printf("Invalid input. Continue [y/n] : ");
            }
        } while(1);
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
            printf("Continue [y/n] : ");
            char response;
            do {
                scanf(" %c", &response);
                if (response == 'y' || response == 'Y') {
                    break;
                } else if (response == 'n' || response == 'N') {
                    return EXIT_FAILURE;
                } else {
                    printf("Invalid input. Continue [y/n] : ");
                }
            } while(1);
    }

    /*
     * Allocate 1 page of memory with MTE protection
     */
    ptr = mmap(NULL, sysconf(_SC_PAGESIZE), PROT_READ | PROT_WRITE | PROT_MTE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (ptr == MAP_FAILED)
    {
        perror("mmap() failed");
        printf("Continue [y/n] : ");
        char response;
        do {
            scanf(" %c", &response);
            if (response == 'y' || response == 'Y') {
                break;
            } else if (response == 'n' || response == 'N') {
                return EXIT_FAILURE;
            } else {
                printf("Invalid input. Continue [y/n] : ");
            }
        } while(1);
    }

    /*
     * Print the pointer value with the default tag (expecting 0)
     */
    printf("pointer is %p\n", ptr);

    /*
     * Write the first 2 bytes of the memory with the default tag
     */
    ptr[0] = 0x41;
    ptr[1] = 0x42;

    /*
     * Read back to confirm the writes
     */
    printf("ptr[0] = 0x%hhx ptr[1] = 0x%hhx\n", ptr[0], ptr[1]);

    /*
     * Generate a random tag and store it for the address (IRG instruction)
     */
    ptr = (unsigned char *) insert_random_tag(ptr);

    /*
     * Set the key on the pointer to match the lock on the memory  (STG instruction)
     */
    set_tag(ptr);

    /*
     * Print the pointer value with the new tag
     */
    printf("pointer is now %p\n", ptr);

    /*
     * Write the first 2 bytes of the memory again, with the new tag
     */
    ptr[0] = 0x43;
    ptr[1] = 0x44;

    /*
     * Read back to confirm the writes
     */
    printf("ptr[0] = 0x%hhx ptr[1] = 0x%hhx\n", ptr[0], ptr[1]);

    /*
     * Write to memory beyond the 16 byte granule (offsest 0x10)
     * MTE should generate an exception
     * If the offset is less than 0x10 no SIGSEGV will occur.
     */
    printf("Expecting SIGSEGV...\n");
    ptr[0x10] = 0x55;

    /*
     * Program only reaches this if no SIGSEGV occurs
     */
    printf("...no SIGSEGV was received\n");

    return EXIT_FAILURE;
}
