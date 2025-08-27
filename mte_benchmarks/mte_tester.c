#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <setjmp.h>
#include <sys/mman.h>
#include <sys/prctl.h>
#include <sys/auxv.h>
#include <errno.h>

/* Some kernels / libc may not expose these; define if missing */
#ifndef PROT_MTE
#define PROT_MTE 0x20
#endif
#ifndef HWCAP2_MTE
#define HWCAP2_MTE (1 << 18)
#endif

/* si_code values for MTE faults */
#ifndef SEGV_MTEAERR
#define SEGV_MTEAERR 8 /* asynchronous */
#endif
#ifndef SEGV_MTESERR
#define SEGV_MTESERR 9 /* synchronous */
#endif

#define PR_SET_TAGGED_ADDR_CTRL 55
#define PR_GET_TAGGED_ADDR_CTRL 56
# define PR_TAGGED_ADDR_ENABLE  (1UL << 0)
# define PR_MTE_TCF_SHIFT       1
# define PR_MTE_TCF_NONE        (0UL << PR_MTE_TCF_SHIFT)
# define PR_MTE_TCF_SYNC        (1UL << PR_MTE_TCF_SHIFT)
# define PR_MTE_TCF_ASYNC       (2UL << PR_MTE_TCF_SHIFT)
# define PR_MTE_TCF_MASK        (3UL << PR_MTE_TCF_SHIFT)
# define PR_MTE_TAG_SHIFT       3
# define PR_MTE_TAG_MASK        (0xffffUL << PR_MTE_TAG_SHIFT)

static sigjmp_buf jmpbuf;

/* volatile sig_atomic_t so it is safe in signal handler */
static volatile sig_atomic_t observed_code = 0;

/* signal handler for SIGSEGV: record si_code and longjmp back */
static void segv_handler(int sig, siginfo_t *si, void *ucontext)
{
    /* only record kernel-provided si_code (positive) */
    observed_code = si ? si->si_code : 0;
    /* resume execution at sigsetjmp (non-zero return) */
    siglongjmp(jmpbuf, 1);
}

/* install the handler */
static void install_handler(void)
{
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_flags = SA_SIGINFO;
    sa.sa_sigaction = segv_handler;
    sigemptyset(&sa.sa_mask);
    if (sigaction(SIGSEGV, &sa, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }
}

static void *alloc_mte_page(void)
{
    size_t ps = sysconf(_SC_PAGESIZE);
    void *p = mmap(NULL, ps, PROT_READ | PROT_WRITE | PROT_MTE,
                   MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (p == MAP_FAILED) {
        perror("mmap(PROT_MTE)");
        exit(1);
    }
    return p;
}

static inline void *tagged_ptr(void *p, uint8_t tag)
{
    uintptr_t a = (uintptr_t)p;
    a = (a & ~((uintptr_t)0xFFULL << 56)) | ((uintptr_t)tag << 56);
    return (void *)a;
}

static int do_access_and_collect(void *addr, int is_write)
{
    observed_code = 0;

    if (sigsetjmp(jmpbuf, 1) == 0) {
        if (is_write) {
            /* attempt write that has mismatched tag */
            volatile uint8_t *p = (volatile uint8_t *)addr;
            *p = 0x42;
        } else {
            /* attempt read that has mismatched tag */
            volatile uint8_t val = *((volatile uint8_t *)addr);
            (void)val;
        }
        /* If no immediate (sync) fault, make a user->kernel transition to
           force delivery of async faults. sleep(0) is a lightweight syscall. */
        sleep(1);

        return observed_code; /* zero if no fault */
    } else {
        /* siglongjmp from handler; observed_code set */
        return observed_code;
    }
}

static void run_mode_test(const char *label, unsigned long prctl_mode_bits,
                          void *probe_ptr,
                          int expected_read_code, int expected_write_code)
{
    int rcode, wcode;

    unsigned long arg = PR_TAGGED_ADDR_ENABLE | prctl_mode_bits | (0xfffeUL << PR_MTE_TAG_SHIFT);

    if (prctl(PR_SET_TAGGED_ADDR_CTRL, arg, 0, 0, 0) != 0) {
        perror("prctl(PR_SET_TAGGED_ADDR_CTRL)");
    }

    printf("\n--- %s ---\n", label);

    /* READ */
    rcode = do_access_and_collect(probe_ptr, 0);
    if (rcode == expected_read_code) {
        printf("[%s][READ] observed expected code %d\n", label, rcode);
    } else if (rcode == 0) {
        printf("[%s][READ] no fault observed (code=0)\n", label);
    } else {
        printf("[%s][READ] unexpected si_code=%d\n", label, rcode);
    }

    /* WRITE */
    wcode = do_access_and_collect(probe_ptr, 1);
    if (wcode == expected_write_code) {
        printf("[%s][WRITE] observed expected code %d\n", label, wcode);
    } else if (wcode == 0) {
        printf("[%s][WRITE] no fault observed (code=0)\n", label);
    } else {
        printf("[%s][WRITE] unexpected si_code=%d\n", label, wcode);
    }

    /* decide success */
    if (rcode == expected_read_code && wcode == expected_write_code) {
        printf("%s MTE: Working\n", label);
    } else {
        printf("%s MTE: FAILED (read=%d, write=%d, expected read=%d, expected write=%d)\n",
               label, rcode, wcode, expected_read_code, expected_write_code);
    }
}

int main(void)
{
    unsigned long hwcap2 = getauxval(AT_HWCAP2);
    if (!(hwcap2 & HWCAP2_MTE)) {
        fprintf(stderr, "This CPU/kernel does not advertise MTE support (HWCAP2). Aborting.\n");
        return 2;
    }

    install_handler();

    void *page = alloc_mte_page();
    void *probe = tagged_ptr(page, 1);

    /* Run ASYNC mode: both read and write should fault asynchronously (SEGV_MTEAERR) */
    run_mode_test("ASYNC",
                  PR_MTE_TCF_ASYNC, /* TCF bits: async */
                  probe,
                  SEGV_MTEAERR, /* expected read */
                  SEGV_MTEAERR  /* expected write */
    );

    /* Run SYNC mode: both read and write should fault synchronously (SEGV_MTESERR) */
    run_mode_test("SYNC",
                  PR_MTE_TCF_SYNC, /* TCF bits: sync */
                  probe,
                  SEGV_MTESERR, /* expected read */
                  SEGV_MTESERR  /* expected write */
    );


    /* Run ASYMMETRIC mode: read = sync, write = async */
    run_mode_test("ASYMM",
                  (PR_MTE_TCF_SYNC | PR_MTE_TCF_ASYNC),
                  probe,
                  SEGV_MTESERR,
                  SEGV_MTEAERR
    );

    return 0;
}
