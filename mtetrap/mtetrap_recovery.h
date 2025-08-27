#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/auxv.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/prctl.h>

#include "mtetrap_module.h"
#include "interval_tree.h"
#include "mtetrap_recoverypatchup.h"

#define check_cond(...) if (!(__VA_ARGS__)) { printf("Check failed in %s:%u\n" #__VA_ARGS__ "\n", __FILE__, __LINE__); abort(); }

//////////////////////////////// Cache Maintanence ////////////////////////////////

static uint32_t extract_cache_line_size(int cache_line_size_shift) {
  uint32_t cache_type_register_;
  asm volatile("mrs %x[ctr], ctr_el0"
                       : [ctr] "=r"(cache_type_register_));
  return 4 << ((cache_type_register_ >> cache_line_size_shift) & 0xF);
}

typedef struct {
  uintptr_t isize, dsize;
  uintptr_t code_start, code_end;
  uintptr_t start_ialigned, start_daligned;
} CacheSyncCtx;

void init_sync_ctx(CacheSyncCtx *pctx, uintptr_t code_start, size_t size) {
  pctx->isize = extract_cache_line_size(0);
  pctx->dsize = extract_cache_line_size(16);
  pctx->code_start = code_start;
  pctx->code_end = code_start + size;
  pctx->start_ialigned = code_start & ~(pctx->isize - 1);
  pctx->start_daligned = code_start & ~(pctx->dsize - 1);
}

CacheSyncCtx mte_sync_ctx;
CacheSyncCtx mprotect_sync_ctx;

//////////////////////////////// MTE ////////////////////////////////

void enable_mte() {
  if (!((getauxval(AT_HWCAP2)) & HWCAP2_MTE)) {
    printf("MTE is not supported\n");
    abort();
  }

  if (prctl(PR_SET_TAGGED_ADDR_CTRL,
            PR_TAGGED_ADDR_ENABLE | PR_MTE_TCF_SYNC | (0xfffe << PR_MTE_TAG_SHIFT),
            0, 0, 0)) {
    perror("prctl() failed");
    abort();
  }
}

//////////////////////////////// General recovery code ////////////////////////////////

// #define mprotect_rerun_inst_offset ((unsigned int) 59*4)
// #define mprotect_recovery_code_size ((unsigned int) 122*4)
// #define mprotect_addr_inst_offset1 ((unsigned int) 26*4)
// #define mprotect_addr_inst_offset2 ((unsigned int) 86*4)
// #define mprotect_sp_restore_inst_offset ((unsigned int) 119*4)
// #define mprotect_br_inst_offset ((unsigned int) 120*4)
//
#define mprotect_rerun_inst_offset ((unsigned int) 63*4)
#define mprotect_recovery_code_size ((unsigned int) 130*4)
#define mprotect_addr_inst_offset1 ((unsigned int) 28*4)
#define mprotect_addr_inst_offset2 ((unsigned int) 92*4)
#define mprotect_sp_restore_inst_offset ((unsigned int) 127*4)
#define mprotect_br_inst_offset ((unsigned int) 128*4)

unsigned int is_setup = 0;
unsigned char * mte_recovery_code_addr = NULL;
unsigned char * mprotect_recovery_code_addr = NULL;

void mte_trap_recovery_func();
void mte_trap_recovery_pre();
void mprotect_trap_recovery_func();

static char* get_page_addr(char* addr) {
  // Drop the bottom 12-bits to find the page
  return (char*) (((uintptr_t) addr) & ((uint64_t)0xfffffffffffff000));
}

unsigned char * setup_recovery_code_helper(
  char* const recovery_func_addr,
  uint64_t dead_inst_offset,
  uint64_t recovery_code_size)
{
  unsigned char * ret = 0;
  const uint32_t faulting = 0x0000dead;

  for (int i = 0; i < 1000; i+=4) {
    if (memcmp(recovery_func_addr + i, &faulting, sizeof(faulting)) == 0) {
      ret = (unsigned char *)recovery_func_addr + i - dead_inst_offset;
      break;
    }
  }

  check_cond(ret != NULL);

  char* recovery_code_end = (char *)ret + recovery_code_size;

  char* startpage_with_recovery_code = get_page_addr((char *)ret);
  char* endpage_with_recovery_code = get_page_addr(recovery_code_end);
  size_t length = (endpage_with_recovery_code - startpage_with_recovery_code) + 4096;

  int mprot_err = mprotect(startpage_with_recovery_code, length, PROT_READ | PROT_WRITE | PROT_EXEC);
  check_cond(mprot_err == 0);

  return ret;
}

void setup_recovery_code() {
  // get_icache_line_size();
  // printf("I-cache line size: %d\n", get_icache_line_size());
  enable_mte();

  mte_recovery_code_addr = setup_recovery_code_helper(
    (char*) &mte_trap_recovery_func,
    mte_rerun_inst_offset,
    mte_recovery_code_size);

  init_sync_ctx(&mte_sync_ctx, (uintptr_t)mte_recovery_code_addr, mte_recovery_code_size);

  mprotect_recovery_code_addr = setup_recovery_code_helper(
    (char*) &mprotect_trap_recovery_func,
    mprotect_rerun_inst_offset,
    mprotect_recovery_code_size);

  // init_sync_ctx(&mprotect_sync_ctx, mprotect_recovery_code_addr, mprotect_recovery_code_size);
}


//////////////////////////////// General signal handler recovery ////////////////////////////////

static int patchup_mte_app_recovery(unsigned char* memory_addr,
  unsigned char** p_inst_addr,
  unsigned char** p_stack_addr,
  unsigned char * recovery_code_addr,
  unsigned long* access_count_addr,
  unsigned char * pre_recovery_code_addr);

typedef void (*signal_handler_t) (int sig, siginfo_t* si, void* arg);

void* g_alt_stack = NULL;

static void install_signal_handler(signal_handler_t handler)
{
  if (!is_setup){
    setup_recovery_code();
    is_setup = 1;
  }

  g_alt_stack = malloc(SIGSTKSZ);
  if (g_alt_stack == NULL) {
    perror("malloc failed");
    abort();
  }

  /* install altstack */
  stack_t ss;
  ss.ss_sp = g_alt_stack;
  ss.ss_flags = 0;
  ss.ss_size = SIGSTKSZ;
  if (sigaltstack(&ss, NULL) != 0) {
    perror("sigaltstack failed");
    abort();
  }

  /* install handler */
  struct sigaction sa;
  memset(&sa, '\0', sizeof(sa));
#ifndef SA_EXPOSE_TAGBITS 
#define SA_EXPOSE_TAGBITS 0x00000800
#endif
  sa.sa_flags = SA_SIGINFO | SA_EXPOSE_TAGBITS;
  sigemptyset(&sa.sa_mask);
  sa.sa_sigaction = handler;

  if (sigaction(SIGSEGV, &sa, NULL) != 0) {
    perror("sigaction failed");
    abort();
  }
}

static void uninstall_signal_handler() {
  /* uninstall altstack */

  /* verify altstack was still in place */
  stack_t ss;
  if (sigaltstack(NULL, &ss) != 0) {
    perror("sigaltstack failed");
    abort();
  }

  if ((!g_alt_stack) || (ss.ss_flags & SS_DISABLE) ||
      (ss.ss_sp != g_alt_stack) || (ss.ss_size != SIGSTKSZ)) {
    printf("warning: alternate stack was modified unexpectedly\n");
    return;
  } else {
    /* disable and free */
    ss.ss_flags = SS_DISABLE;
    if (sigaltstack(&ss, NULL) != 0) {
      perror("sigaltstack failed");
      abort();
    }
    free(g_alt_stack);
    g_alt_stack = NULL;
  }

  /* uninstall handler */
  struct sigaction sa;
  memset(&sa, '\0', sizeof(sa));
  sa.sa_handler = SIG_DFL;
  if (sigaction(SIGSEGV, &sa, NULL) != 0) {
    perror("sigaction failed");
    abort();
  }
}

//////////////////////////////// MTE Signal-based recovery ////////////////////////////////

struct interval_tree g_mte_bytes_watched = {0};

uint64_t mte_access_count = 0;

static void os_mte_signal_handler(int sig, siginfo_t* si, void* arg) {
  check_cond(sig == SIGSEGV);

  ucontext_t *context = (ucontext_t *)arg;

  check_cond(si->si_code == SEGV_MTESERR);

  unsigned char* original_memory_addr = (unsigned char*) si->si_addr;
  char* const inst_addr = (char*) context->uc_mcontext.pc;
  char* const stack_ptr = (char*) context->uc_mcontext.sp;

  // printf("!!!ADDR: %p, PC: %p, SP: %p\n", original_memory_addr, inst_addr, stack_ptr);

  int patchup_failed = patchup_mte_app_recovery(original_memory_addr,
    (unsigned char**) &(context->uc_mcontext.pc),
    (unsigned char**) &(context->uc_mcontext.sp),
    mte_recovery_code_addr,
    &mte_access_count,
    (unsigned char *) &mte_trap_recovery_pre
    );

  // asm volatile("isb");

  check_cond(patchup_failed == 0);
}

void install_signal_based_recovery() {
  mte_access_count = 0;
  install_signal_handler(&os_mte_signal_handler);
}

void uninstall_signal_based_recovery() {
  uninstall_signal_handler();
}

static void set_mte_data_watchpoint_tag(unsigned char * ptr, size_t length, unsigned char tag) {
  unsigned char * tagged_ptr = (unsigned char *) insert_chosen_tag(ptr, tag);
  printf("[%s] tag: 0x%x -> 0x%x, addr: %p, len: %ld\n", __func__, (unsigned char)((uintptr_t)ptr >> 56), tag, ptr, length);
  if ((uintptr_t)ptr >> 56 != 0) {
    perror("given memory is already tagged");
    abort();
  }

  // NOTE: stg only takes aligned addresses.
  for (size_t i = 0; i < length; i += 16) {
    asm volatile("stg %0, [%0]" : : "r" (tagged_ptr + i) : "memory"); \
  }
}

void add_mte_data_watchpoint(unsigned char * ptr, size_t length) {
  const uint64_t ptr_end = (uint64_t) (ptr + length - 1);
  add_interval(&g_mte_bytes_watched, (uint64_t) ptr, ptr_end);

  const uint64_t page_size = 4096;
  const uint64_t page_mask = page_size - 1;
  const uint64_t pages_start = ((uint64_t)ptr) & ~page_mask;
  const uint64_t pages_end = (ptr_end & ~page_mask) + page_mask;

  if (mprotect((void*) pages_start, pages_end - pages_start + 1, PROT_READ | PROT_WRITE | PROT_MTE) != 0) {
    perror("mprotect failed");
    abort();
  }

  set_mte_data_watchpoint_tag(ptr, length, data_watchpoint_tag);
}

static void reset_mte_perms(void* param, uint64_t start, uint64_t end) {
  set_mte_data_watchpoint_tag((unsigned char *) start, end - start + 1, 0);
}

void remove_all_mte_data_watchpoints() {
  // Reset the page perms
  for_each_interval(&g_mte_bytes_watched, 0 /* unused param */, &reset_mte_perms);
  memset(&g_mte_bytes_watched, 0, sizeof(g_mte_bytes_watched));
  printf("Accessed %lu times\n\n", mte_access_count);
  mte_access_count = 0;
}

//////////////////////////////// MTE Module-based recovery ////////////////////////////////

static int patchup_mprotect_app_recovery(unsigned char* memory_addr,
  unsigned char** p_inst_addr,
  unsigned char** p_stack_addr,
  unsigned char * recovery_code_addr)
{
  // doprint("Recovery: stage 0\n");
  unsigned long pgsize = sysconf(_SC_PAGESIZE);

  const unsigned long memory_addr_val = (unsigned long) memory_addr;
  unsigned char* stack_ptr = *p_stack_addr;
  unsigned char* inst_addr = *p_inst_addr;

  const unsigned short ptr_2byte_1 = (unsigned short)(memory_addr_val & ~(pgsize - 1));
  const unsigned short ptr_2byte_2 = (unsigned short)(memory_addr_val >> 16);
  const unsigned short ptr_2byte_3 = (unsigned short)(memory_addr_val >> 32);
  const unsigned short ptr_2byte_4 = (unsigned short)(memory_addr_val >> 48);

  const unsigned int ptr_shifted_1 = 0x06940000 | ((unsigned int)ptr_2byte_1);
  const unsigned int ptr_shifted_2 = 0x07950000 | ((unsigned int)ptr_2byte_2);
  const unsigned int ptr_shifted_3 = 0x07960000 | ((unsigned int)ptr_2byte_3);
  const unsigned int ptr_shifted_4 = 0x07970000 | ((unsigned int)ptr_2byte_4);

  // Begin the recovery process
  const unsigned int mprotect_addr_inst [] = {
    ptr_shifted_1 << 5,
    ptr_shifted_2 << 5,
    ptr_shifted_3 << 5,
    ptr_shifted_4 << 5,
  };

  const long min_offset = -0x7ffffff;
  const long max_offset =  0x7ffffff;
  const unsigned long offset_mask = 0xfffffff;

  // Vars used later
  unsigned int faulting_inst_copy = 0;
  unsigned char* new_stack_ptr = 0;
  unsigned char* next_instr_addr = 0;
  unsigned char* br_inst_addr = 0;
  long offset = 0;
  unsigned long offset_compressed = 0;
  unsigned int br_rel_inst = 0;
  unsigned char stack_immediate = 0;

  // 1. Copy the address into the recovery page
  // doprint("Recovery: stage 1\n");
  write_program_addr(&(recovery_code_addr[mprotect_addr_inst_offset1]), mprotect_addr_inst, sizeof(mprotect_addr_inst));
  write_program_addr(&(recovery_code_addr[mprotect_addr_inst_offset2]), mprotect_addr_inst, sizeof(mprotect_addr_inst));

  // 2. Copy the faulting instruction to the recovery page
  // doprint("Recovery: stage 2\n");
  read_program_addr(&faulting_inst_copy, inst_addr, sizeof(unsigned int));
  write_program_addr(&(recovery_code_addr[mprotect_rerun_inst_offset]), &faulting_inst_copy, sizeof(unsigned int));

  // 3. Align the stack
  // doprint("Recovery: stage 3\n");
  new_stack_ptr = stack_ptr;
  stack_immediate = 0;
  if (((unsigned long)new_stack_ptr) % 16 != 0) {
    stack_immediate += 8;
    new_stack_ptr -= 8;
  }
  *p_stack_addr = new_stack_ptr;

  // The stack room immediate is a 12-bit constant stored in bits 21 to 10 in the instruction at mprotect_sp_restore_inst_offset
  // However, since we only want write values 0 (0000) or 8 (1000), we can restrict our writes from bit 10 to bit 15
  // Additionally, bits 8 and 9 are fixed at 1.
  // So we just have to write a single uint8_t from bits 8 to 16
  stack_immediate = (stack_immediate << 2) | ((unsigned char)3);
  write_program_addr(&(recovery_code_addr[mprotect_sp_restore_inst_offset + 1]), &stack_immediate, sizeof(stack_immediate));

  // 4. Compute and write the offset of the PC after the faulting instruction so we can return to it
  // doprint("Recovery: stage 4\n");
  next_instr_addr = inst_addr + 4;
  br_inst_addr = &(recovery_code_addr[mprotect_br_inst_offset]);
  offset = next_instr_addr - br_inst_addr;
  if(offset < min_offset || offset > max_offset) {
    return 1;
  }

  offset_compressed = (((unsigned long) offset) & offset_mask) >> 2;
  br_rel_inst = 0x14000000 | ((unsigned int) offset_compressed);
  write_program_addr(br_inst_addr, &br_rel_inst, sizeof(br_rel_inst));

  // 5. Set the PC to resume as the recovery page
  // doprint("Recovery: stage 5\n");
  *p_inst_addr = recovery_code_addr;

  // doprint("Recovery: finished\n");
  __builtin___clear_cache((void *)recovery_code_addr, (void *)((uintptr_t)recovery_code_addr + mprotect_recovery_code_size));

  return 0;
}

void install_module_based_recovery()
{
  if (!is_setup){
    setup_recovery_code();
    is_setup = 1;
  }
  // asm volatile ("prfm plil1keep, [%0]" : : "r"(mte_recovery_code_addr));

  mte_access_count = 0;

  // puts("Querying " MTETRAP_DEVICE_PATH ".\n");

  int mtef = open(MTETRAP_DEVICE_PATH, 0);
  if(mtef < 0) {
    printf("Can't find mte trap device. Please insmod mtetrap_module.ko first.");
    exit(1);
  }

  printf("Running MTETRAP_IOCTL_CMD_SETREDIRECT with handler: %p, %p\n", mte_recovery_code_addr, &mte_trap_recovery_pre);
  struct ioctl_setredirect_params param;
  param.trap_handler = (unsigned long) mte_recovery_code_addr;
  param.access_count_addr = (unsigned long) &mte_access_count;
  param.trap_handler_pre = (unsigned long) &mte_trap_recovery_pre;

  if(ioctl(mtef, MTETRAP_IOCTL_CMD_SETREDIRECT, &param) < 0) {
    printf("Failed to execute mtetrap setredirect.\n");
    exit(1);
  }

  close(mtef);
}

void uninstall_module_based_recovery()
{
  // puts("Querying " MTETRAP_DEVICE_PATH ".\n");

  int mtef = open(MTETRAP_DEVICE_PATH, 0);
  if(mtef < 0) {
    printf("Can't find mte trap device. Please insmod mtetrap_module.ko first.");
    exit(1);
  }

  // printf("Running MTETRAP_IOCTL_CMD_SETREDIRECT with handler: 0\n");

  if(ioctl(mtef, MTETRAP_IOCTL_CMD_SETREDIRECT, 0) < 0) {
    printf("Failed to execute mtetrap setpid.\n");
    exit(1);
  }

  close(mtef);
}


//////////////////////////////// Mprotect-based recovery ////////////////////////////////

struct interval_tree g_mprotect_bytes_watched = {0};
struct interval_tree g_mprotect_pages_watched = {0};

uint64_t mprotect_access_count = 0;
uint64_t mprotect_access_falarm_count = 0;

static void os_mprotect_signal_handler(int sig, siginfo_t* si, void* arg) {
  check_cond(sig == SIGSEGV);

  unsigned char* original_memory_addr = (unsigned char*) si->si_addr;
  int in_watched_pages = is_in_interval(&g_mprotect_pages_watched, (uintptr_t) original_memory_addr);

  if (!in_watched_pages) {
    printf("Got a sigsegv not in watched pages\n");
    abort();
  }

  int in_watched_interval = is_in_interval(&g_mprotect_bytes_watched, (uintptr_t) original_memory_addr);
  if (in_watched_interval) {
    mprotect_access_count++;
  } else {
    mprotect_access_falarm_count++;
  }

  // Recovery
  ucontext_t *context = (ucontext_t *)arg;

  char* const inst_addr = (char*) context->uc_mcontext.pc;
  char* const stack_ptr = (char*) context->uc_mcontext.sp;

  // printf("!!!ADDR: %p, PC: %p, SP: %p\n", original_memory_addr, inst_addr, stack_ptr);

  int patchup_failed = patchup_mprotect_app_recovery(original_memory_addr,
    (unsigned char**) &(context->uc_mcontext.pc),
    (unsigned char**) &(context->uc_mcontext.sp),
    mprotect_recovery_code_addr);

  check_cond(patchup_failed == 0);
}

void install_mprotect_based_recovery() {
  mprotect_access_count = 0;
  install_signal_handler(&os_mprotect_signal_handler);
}

void uninstall_mprotect_based_recovery() {
  uninstall_signal_handler();
}

void add_mprotect_data_watchpoint(unsigned char * ptr, size_t length) {
  const uint64_t ptr_end = (uint64_t) (ptr + length - 1);
  add_interval(&g_mprotect_bytes_watched, (uint64_t) ptr, ptr_end);

  const uint64_t page_size = 4096;
  const uint64_t page_mask = page_size - 1;
  const uint64_t pages_start = ((uint64_t)ptr) & ~page_mask;
  const uint64_t pages_end = (ptr_end & ~page_mask) + page_mask;

  if (mprotect((void*) pages_start, pages_end - pages_start + 1, PROT_NONE) != 0) {
    perror("mprotect failed");
    abort();
  }

  add_interval(&g_mprotect_pages_watched, pages_start, pages_end);
}

static void reset_page_perms(void* param, uint64_t start, uint64_t end) {
  if (mprotect((void*) start, end - start + 1, PROT_READ|PROT_WRITE) != 0) {
    perror("mprotect failed");
    abort();
  }
}

void remove_all_mprotect_data_watchpoints() {
  // Reset the page perms
  for_each_interval(&g_mprotect_pages_watched, 0 /* unused param */, &reset_page_perms);
  memset(&g_mprotect_pages_watched, 0, sizeof(g_mprotect_pages_watched));
  memset(&g_mprotect_bytes_watched, 0, sizeof(g_mprotect_bytes_watched));

  printf("Page-accessed %lu times\n\n", mprotect_access_count);
  mprotect_access_count = 0;
  printf("Page-accessed %lu times (false alarm)\n\n", mprotect_access_falarm_count);
  mprotect_access_falarm_count = 0;
}

//////////////////////////////// More general recovery code ////////////////////////////////

void mte_trap_recovery_pre() {
  asm volatile(
      "stp x0, x1, [sp, #-0x10]!\n"
      "stp x2, x3, [sp, #-0x10]!\n"
      "stp x4, x5, [sp, #-0x10]!\n"
      "adrp x4, mte_sync_ctx\n" // &pctx, avoid truncate error
      "add x4, x4, :lo12:mte_sync_ctx\n"
      "ldr x0, [x4, %0]\n" // dline_size
      "ldr x1, [x4, %1]\n" // iline_size

      "ldr x2, [x4, %2]\n" // start_Dalinged
      "ldr x3, [x4, %3]\n" // end
      "0:\n"
      "dc   cvau, x2\n"
      "add  x2, x2, x0\n" // += dline_size
      "cmp  x2, x3\n"
      "b.lt 0b\n"

      "dsb ish\n"

      "ldr x2, [x4, %4]\n" // start_Ialinged
      "1:\n"
      "ic   ivau, x2\n"
      "add  x2, x2, x1\n" // += iline_size
      "cmp  x2, x3\n"
      "b.lt 1b\n"

      "dsb ish\n"

      "isb\n"

      "ldp x4, x5, [sp], #0x10\n"
      "ldp x2, x3, [sp], #0x10\n"
      "ldp x0, x1, [sp], #0x10\n"
      "b mte_trap_recovery_func\n"
      :
      : "i"(offsetof(CacheSyncCtx, dsize)),
        "i"(offsetof(CacheSyncCtx, isize)),
        "i"(offsetof(CacheSyncCtx, start_daligned)),
        "i"(offsetof(CacheSyncCtx, code_end)),
        "i"(offsetof(CacheSyncCtx, start_ialigned))
    );
}

void mte_trap_recovery_func() {
  asm volatile(
    "str  x0, [sp]\n"
    // x0 will be filled with the untagged memory address
    // Note: uint16_t constant goes from bit 20 to 5 in next 4 instructions
    "mov  x0, #0xdef0\n"
    "movk x0, #0x9abc, lsl #16\n"
    "movk x0, #0x5678, lsl #32\n"
    "movk x0, #0x1234, lsl #48\n"
    "stg  x0, [x0]\n"
    // WARN: this occasionally untags more than enough
    // "stg  x0, [x0, #16]\n"
    "ldr x0, [sp]\n"
    // encoding "0x0000dead". replaced by faulting instruction
    "udf  0xdead\n"
    "str  x0, [sp]\n"
    // x0 will be filled with the tagged memory address
    // Note: uint16_t constant goes from bit 20 to 5 in next 4 instructions
    "mov  x0, #0xdef0\n"
    "movk x0, #0x9abc, lsl #16\n"
    "movk x0, #0x5678, lsl #32\n"
    "movk x0, #0x1234, lsl #48\n"
    "stg  x0, [x0]\n"
    // WARN: this may falsely tag adjacent not-traced data at the tag boundary
    // "stg  x0, [x0, #16]\n"
    "ldr x0, [sp]\n"
    // Note: uint12_t immediate goes from bit 21 to 10
    "add	sp, sp, #0x8\n"
    // relative 28-bit signed offset that is right-shifted by 2, is placed in bits 31 to 6
    "b 0\n"
    "ret\n"
  );
}

#define ARM_CALLER_SAVE           \
  "stp x0,  x1 , [sp, #-0x10]!\n" \
  "stp x2,  x3 , [sp, #-0x10]!\n" \
  "stp x4,  x5 , [sp, #-0x10]!\n" \
  "stp x6,  x7 , [sp, #-0x10]!\n" \
  "stp x8,  x9 , [sp, #-0x10]!\n" \
  "stp x10, x11, [sp, #-0x10]!\n" \
  "stp x12, x13, [sp, #-0x10]!\n" \
  "stp x14, x15, [sp, #-0x10]!\n" \
  "stp x16, x17, [sp, #-0x10]!\n" \
  "stp x18, x30, [sp, #-0x10]!\n" \
  "stp d0,  d1 , [sp, #-0x20]!\n" \
  "stp d2,  d3 , [sp, #-0x20]!\n" \
  "stp d4,  d5 , [sp, #-0x20]!\n" \
  "stp d6,  d7 , [sp, #-0x20]!\n" \
  "stp d8 , d9 , [sp, #-0x20]!\n" \
  "stp d10, d11, [sp, #-0x20]!\n" \
  "stp d12, d13, [sp, #-0x20]!\n" \
  "stp d14, d15, [sp, #-0x20]!\n" \
  "stp d16, d17, [sp, #-0x20]!\n" \
  "stp d18, d19, [sp, #-0x20]!\n" \
  "stp d20, d21, [sp, #-0x20]!\n" \
  "stp d22, d23, [sp, #-0x20]!\n" \
  "stp d24, d25, [sp, #-0x20]!\n" \
  "stp d26, d27, [sp, #-0x20]!\n" \
  "stp d28, d29, [sp, #-0x20]!\n" \
  "stp d30, d31, [sp, #-0x20]!\n" \
  "mrs x0, nzcv\n" \
  "str x0, [sp, #-0x10]!\n"

#define ARM_CALLER_RESTORE        \
    "ldr x0, [sp], #0x10\n"  \
    "msr nzcv, x0\n"               \
    "ldp d30, d31, [sp], #0x20\n"  \
    "ldp d28, d29, [sp], #0x20\n"  \
    "ldp d26, d27, [sp], #0x20\n"  \
    "ldp d24, d25, [sp], #0x20\n"  \
    "ldp d22, d23, [sp], #0x20\n"  \
    "ldp d20, d21, [sp], #0x20\n"  \
    "ldp d18, d19, [sp], #0x20\n"  \
    "ldp d16, d17, [sp], #0x20\n"  \
    "ldp d14, d15, [sp], #0x20\n"  \
    "ldp d12, d13, [sp], #0x20\n"  \
    "ldp d10, d11, [sp], #0x20\n"  \
    "ldp d8 , d9 , [sp], #0x20\n"  \
    "ldp d6,  d7 , [sp], #0x20\n"  \
    "ldp d4,  d5 , [sp], #0x20\n"  \
    "ldp d2,  d3 , [sp], #0x20\n"  \
    "ldp d0,  d1 , [sp], #0x20\n"  \
    "ldp x18, x30, [sp], #0x10\n"  \
    "ldp x16, x17, [sp], #0x10\n"  \
    "ldp x14, x15, [sp], #0x10\n"  \
    "ldp x12, x13, [sp], #0x10\n"  \
    "ldp x10, x11, [sp], #0x10\n"  \
    "ldp x8,  x9 , [sp], #0x10\n"  \
    "ldp x6,  x7 , [sp], #0x10\n"  \
    "ldp x4,  x5 , [sp], #0x10\n"  \
    "ldp x2,  x3 , [sp], #0x10\n"  \
    "ldp x0,  x1 , [sp], #0x10\n"

void mprotect_trap_recovery_func() {
  asm volatile(
    // Save caller-save registers
    ARM_CALLER_SAVE
    // Load mprotect params
    //    x0 will be filled with the target memory address
    //    Note: uint16_t constant goes from bit 20 to 5 in next 4 instructions
    "mov  x0, #0xdef0\n"
    "movk x0, #0x9abc, lsl #16\n"
    "movk x0, #0x5678, lsl #32\n"
    "movk x0, #0x1234, lsl #48\n"
    //    x1 is Length which is 1 page
    "mov x1, #0x1000\n"
    //    Perform mprotect with PROT_READ|PROT_WRITE
    "mov x2, #3\n"
    // Invoke mprotect
    "bl mprotect\n"
    // Restore caller-save registers
    ARM_CALLER_RESTORE

    // encoding "0x0000dead". replaced by faulting instruction
    "udf  0xdead\n"

    // Save caller-save registers
    ARM_CALLER_SAVE
    // Load mprotect params
    //    x0 will be filled with the target memory address
    //    Note: uint16_t constant goes from bit 20 to 5 in next 4 instructions
    "mov  x0, #0xdef0\n"
    "movk x0, #0x9abc, lsl #16\n"
    "movk x0, #0x5678, lsl #32\n"
    "movk x0, #0x1234, lsl #48\n"
    //    x1 is Length which is 1 page
    "mov x1, #0x1000\n"
    //    Perform mprotect with PROT_NONE
    "mov x2, #0\n"
    // Invoke mprotect
    "bl mprotect\n"
    // Restore caller-save registers
    ARM_CALLER_RESTORE

    // Note: uint12_t immediate goes from bit 21 to 10
    "add	sp, sp, #0x8\n"
    // relative 28-bit signed offset that is right-shifted by 2, is placed in bits 31 to 6
    "b 0\n"
    "ret\n"
  );
}
