/*
 * Copyright 2018 WebAssembly Community Group participants
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "wasm-rt-impl.h"

#include <assert.h>
#include <math.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if WASM_RT_INSTALL_SIGNAL_HANDLER && !defined(_WIN32)
#include <signal.h>
#include <unistd.h>
#endif

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/mman.h>
#endif

#if WASM_RT_USE_SEGUE || WASM_RT_USE_SHADOW_SEGUE || WASM_RT_USE_SEGUE_LOAD || WASM_RT_USE_SEGUE_STORE
#include <immintrin.h>
#endif

#if WASM_RT_USE_MTE
#include <unistd.h>
#include <sys/auxv.h>
#include <sys/mman.h>
#include <sys/prctl.h>
#define PROT_MTE_LOCALDEF                 0x20
#endif

#define PAGE_SIZE 65536
#define OSPAGE_SIZE 4096

#if WASM_RT_INSTALL_SIGNAL_HANDLER
static bool g_signal_handler_installed = false;
#ifdef _WIN32
static void* g_sig_handler_handle = 0;
#else
static char* g_alt_stack = 0;
#endif
#endif

#if WASM_RT_USE_STACK_DEPTH_COUNT
WASM_RT_THREAD_LOCAL uint32_t wasm_rt_call_stack_depth;
WASM_RT_THREAD_LOCAL uint32_t wasm_rt_saved_call_stack_depth;
#endif

WASM_RT_THREAD_LOCAL wasm_rt_jmp_buf g_wasm_rt_jmp_buf;

#ifdef WASM_RT_TRAP_HANDLER
extern void WASM_RT_TRAP_HANDLER(wasm_rt_trap_t code);
#endif

#ifdef WASM_RT_GROW_FAILED_HANDLER
extern void WASM_RT_GROW_FAILED_HANDLER();
#endif

void wasm_rt_trap(wasm_rt_trap_t code) {
  assert(code != WASM_RT_TRAP_NONE);
#if WASM_RT_USE_STACK_DEPTH_COUNT
  wasm_rt_call_stack_depth = wasm_rt_saved_call_stack_depth;
#endif

#ifdef WASM_RT_TRAP_HANDLER
  WASM_RT_TRAP_HANDLER(code);
  wasm_rt_unreachable();
#else
  WASM_RT_LONGJMP(g_wasm_rt_jmp_buf, code);
#endif
}

#ifdef _WIN32
static void* os_mmap(size_t size) {
  void* ret = VirtualAlloc(NULL, size, MEM_RESERVE, PAGE_NOACCESS);
  return ret;
}

static int os_munmap(void* addr, size_t size) {
  // Windows can only unmap the whole mapping
  (void)size; /* unused */
  BOOL succeeded = VirtualFree(addr, 0, MEM_RELEASE);
  return succeeded ? 0 : -1;
}

static int os_mprotect(void* addr, size_t size) {
  if (size == 0) {
    return 0;
  }
  void* ret = VirtualAlloc(addr, size, MEM_COMMIT, PAGE_READWRITE);
  if (ret == addr) {
    return 0;
  }
  VirtualFree(addr, 0, MEM_RELEASE);
  return -1;
}

static int os_mprotect_read(void* addr, size_t size) {
  if (size == 0) {
    return 0;
  }
  void* ret = VirtualAlloc(addr, size, MEM_COMMIT, PAGE_READ);
  if (ret == addr) {
    return 0;
  }
  VirtualFree(addr, 0, MEM_RELEASE);
  return -1;
}

static void os_print_last_error(const char* msg) {
  DWORD errorMessageID = GetLastError();
  if (errorMessageID != 0) {
    LPSTR messageBuffer = 0;
    // The api creates the buffer that holds the message
    size_t size = FormatMessageA(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
            FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL, errorMessageID, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPSTR)&messageBuffer, 0, NULL);
    (void)size;
    printf("%s. %s\n", msg, messageBuffer);
    LocalFree(messageBuffer);
  } else {
    printf("%s. No error code.\n", msg);
  }
}

#if WASM_RT_INSTALL_SIGNAL_HANDLER

static LONG os_signal_handler(PEXCEPTION_POINTERS info) {
  if (info->ExceptionRecord->ExceptionCode == EXCEPTION_ACCESS_VIOLATION) {
    wasm_rt_trap(WASM_RT_TRAP_OOB);
  } else if (info->ExceptionRecord->ExceptionCode == EXCEPTION_STACK_OVERFLOW) {
    wasm_rt_trap(WASM_RT_TRAP_EXHAUSTION);
  }
  return EXCEPTION_CONTINUE_SEARCH;
}

static void os_install_signal_handler(void) {
  g_sig_handler_handle =
      AddVectoredExceptionHandler(1 /* CALL_FIRST */, os_signal_handler);
}

static void os_cleanup_signal_handler(void) {
  RemoveVectoredExceptionHandler(g_sig_handler_handle);
}

#endif

#else
static void* os_mmap(size_t size) {
  int map_prot = PROT_NONE;
#if WASM_RT_USE_MTE
    map_prot = map_prot | PROT_MTE_LOCALDEF;
#endif
  int map_flags = MAP_ANONYMOUS | MAP_PRIVATE;
  uint8_t* addr = mmap(NULL, size, map_prot, map_flags, -1, 0);
  if (addr == MAP_FAILED)
    return NULL;
  return addr;
}
static void* os_mmap_read(size_t size) {
  int map_prot = PROT_READ;
#if WASM_RT_USE_MTE
    map_prot = map_prot | PROT_MTE_LOCALDEF;
#endif
  int map_flags = MAP_ANONYMOUS | MAP_PRIVATE;
  uint8_t* addr = mmap(NULL, size, map_prot, map_flags, -1, 0);
  if (addr == MAP_FAILED)
    return NULL;
  return addr;
}

static int os_munmap(void* addr, size_t size) {
  return munmap(addr, size);
}

static int os_mprotect(void* addr, size_t size) {
  int map_prot = PROT_READ | PROT_WRITE;
#if WASM_RT_USE_MTE
    map_prot = map_prot | PROT_MTE_LOCALDEF;
#endif
  return mprotect(addr, size, map_prot);
}

static int os_mprotect_read(void* addr, size_t size) {
  int map_prot = PROT_READ;
#if WASM_RT_USE_MTE
    map_prot = map_prot | PROT_MTE_LOCALDEF;
#endif
  return mprotect(addr, size, map_prot);
}

static void os_print_last_error(const char* msg) {
  perror(msg);
}

#if WASM_RT_INSTALL_SIGNAL_HANDLER
static void os_signal_handler(int sig, siginfo_t* si, void* unused) {
  if (si->si_code == SEGV_ACCERR) {
    wasm_rt_trap(WASM_RT_TRAP_OOB);
  } else {
    wasm_rt_trap(WASM_RT_TRAP_EXHAUSTION);
  }
}

static void os_install_signal_handler(void) {
  /* Use alt stack to handle SIGSEGV from stack overflow */
  g_alt_stack = malloc(SIGSTKSZ);
  if (g_alt_stack == NULL) {
    perror("malloc failed");
    abort();
  }

  stack_t ss;
  ss.ss_sp = g_alt_stack;
  ss.ss_flags = 0;
  ss.ss_size = SIGSTKSZ;
  if (sigaltstack(&ss, NULL) != 0) {
    perror("sigaltstack failed");
    abort();
  }

  struct sigaction sa;
  memset(&sa, '\0', sizeof(sa));
  sa.sa_flags = SA_SIGINFO | SA_ONSTACK;
  sigemptyset(&sa.sa_mask);
  sa.sa_sigaction = os_signal_handler;

  /* Install SIGSEGV and SIGBUS handlers, since macOS seems to use SIGBUS. */
  if (sigaction(SIGSEGV, &sa, NULL) != 0 || sigaction(SIGBUS, &sa, NULL) != 0) {
    perror("sigaction failed");
    abort();
  }
}

static void os_cleanup_signal_handler(void) {
  /* Undo what was done in os_install_signal_handler */
  struct sigaction sa;
  memset(&sa, '\0', sizeof(sa));
  sa.sa_handler = SIG_DFL;
  if (sigaction(SIGSEGV, &sa, NULL) != 0 || sigaction(SIGBUS, &sa, NULL)) {
    perror("sigaction failed");
    abort();
  }

  if (sigaltstack(NULL, NULL) != 0) {
    perror("sigaltstack failed");
    abort();
  }

  free(g_alt_stack);
}
#endif

#endif

void wasm_rt_init(void) {
#if WASM_RT_INSTALL_SIGNAL_HANDLER
  if (!g_signal_handler_installed) {
    g_signal_handler_installed = true;
    os_install_signal_handler();
  }
#endif

#if WASM_RT_USE_MTE
  if (prctl(PR_SET_TAGGED_ADDR_CTRL,
            PR_TAGGED_ADDR_ENABLE | PR_MTE_TCF_SYNC | (0xfffe << PR_MTE_TAG_SHIFT),
            0, 0, 0))
  {
    perror("prctl() failed");
    abort();
  }
#endif
}

bool wasm_rt_is_initialized(void) {
#if WASM_RT_INSTALL_SIGNAL_HANDLER
  return g_signal_handler_installed;
#else
  return true;
#endif
}

void wasm_rt_free(void) {
#if WASM_RT_INSTALL_SIGNAL_HANDLER
  os_cleanup_signal_handler();
  g_signal_handler_installed = false;
#endif
}

#if WASM_RT_USE_MMAP

static uint64_t get_allocation_size_for_mmap(wasm_rt_memory_t* memory) {
  assert(!memory->is64 &&
         "memory64 is not yet compatible with WASM_RT_USE_MMAP");
#if WASM_RT_MEMCHECK_GUARD_PAGES
  /* Reserve 8GiB. */
  const uint64_t max_size = 0x200000000ul;
  return max_size;
#else
  if (memory->max_pages != 0) {
    const uint64_t max_size = memory->max_pages * PAGE_SIZE;
    return max_size;
  }

  /* Reserve 4GiB. */
  const uint64_t max_size = 0x100000000ul;
  return max_size;
#endif
}

#endif

#if WASM_RT_MEMCHECK_SHADOW_BYTES || WASM_RT_MEMCHECK_PRESHADOW_BYTES || WASM_RT_MEMCHECK_SHADOW_BYTES_TAG || WASM_RT_MEMCHECK_SHADOW_PAGE || WASM_RT_MEMCHECK_PRESHADOW_PAGE

static uint64_t div_and_roundup(size_t numerator, size_t denominator) {
  if (numerator == 0) {
    return 1;
  }

  uint64_t pages = ((numerator - 1) / denominator) + 1;
  return pages;
}
#endif

#if WASM_RT_MEMCHECK_SHADOW_BYTES || WASM_RT_MEMCHECK_PRESHADOW_BYTES || WASM_RT_MEMCHECK_SHADOW_BYTES_TAG
static size_t get_required_shadow_bytes(size_t pages) {
#if WASM_RT_MEMCHECK_PRESHADOW_BYTES ||          \
    WASM_RT_MEMCHECK_SHADOW_BYTES_TAG ||        \
    WASM_RT_MEMCHECK_SHADOW_BYTES_SCHEME == 1 || \
    WASM_RT_MEMCHECK_SHADOW_BYTES_SCHEME == 2 || \
    WASM_RT_MEMCHECK_SHADOW_BYTES_SCHEME == 5 || \
    WASM_RT_MEMCHECK_SHADOW_BYTES_SCHEME == 6
  // 1 shadow cell per 4gb
  const size_t wasm_pages_per_shadow_cell = 1ULL << 16;
#elif WASM_RT_MEMCHECK_SHADOW_BYTES_SCHEME == 3 || \
    WASM_RT_MEMCHECK_SHADOW_BYTES_SCHEME == 7
  // 1 shadow cell per 64kb
  const size_t wasm_pages_per_shadow_cell = 1ULL;
#else
  // 1 shadow cell per 16mb
  const size_t wasm_pages_per_shadow_cell = 1ULL << 8;  // 16mb
#endif

  const size_t shadow_cell_size =
      sizeof(((wasm_rt_memory_t*)NULL)->shadow_bytes[0]);
  size_t ret =
      div_and_roundup(pages, wasm_pages_per_shadow_cell) * shadow_cell_size;
  return ret;
}

#endif

#if WASM_RT_MEMCHECK_SHADOW_BYTES || WASM_RT_MEMCHECK_PRESHADOW_BYTES
static void* get_shadow_bytes_pointer(uint8_t* shadow_bytes_allocation,
                                      size_t shadow_bytes_allocation_size,
                                      uint64_t pages) {
  const size_t accessible_size = shadow_bytes_allocation_size / 2;
  uint8_t* inaccessible_shadow_ptr = shadow_bytes_allocation + accessible_size;
  size_t accessible_shadow_bytes = get_required_shadow_bytes(pages);
  uint8_t* shadow_ptr = inaccessible_shadow_ptr - accessible_shadow_bytes;
  return shadow_ptr;
}

#endif

#if WASM_RT_MEMCHECK_MPX

struct mpx_xsave_hdr_struct
{
    uint64_t xstate_bv;
    uint64_t reserved1[2];
    uint64_t reserved2[5];
} __attribute__((packed));

struct mpx_bndregs_struct
{
    uint64_t bndregs[8];
} __attribute__((packed));

struct mpx_bndcsr_struct
{
    uint64_t cfg_reg_u;
    uint64_t status_reg;
} __attribute__((packed));

struct mpx_xsave_struct
{
    uint8_t fpu_sse[512];
    struct mpx_xsave_hdr_struct xsave_hdr;
    uint8_t ymm[256];
    uint8_t lwp[128];
    struct mpx_bndregs_struct bndregs;
    struct mpx_bndcsr_struct bndcsr;
} __attribute__((packed));

static inline void mpx_xrstor_state(struct mpx_xsave_struct *fx, uint64_t mask)
{
    uint32_t lmask = mask;
    uint32_t hmask = mask >> 32;

#ifdef __i386__
#define REX_PREFIX
#else /* __i386__ */
#define REX_PREFIX "0x48, "
#endif

    asm volatile(".byte " REX_PREFIX "0x0f,0xae,0x2f\n\t"
                 : : "D"(fx), "m"(*fx), "a"(lmask), "d"(hmask)
                 : "memory");
#undef REX_PREFIX
}

static bool enable_manual_mpx()
{
    uint8_t __attribute__((__aligned__(64))) buffer[4096];
    struct mpx_xsave_struct *xsave_buf = (struct mpx_xsave_struct *)buffer;

    memset(buffer, 0, sizeof(buffer));
    mpx_xrstor_state(xsave_buf, 0x18);

    /* Enable MPX.  */
    xsave_buf->xsave_hdr.xstate_bv = 0x10;
    xsave_buf->bndcsr.cfg_reg_u = 0; // no spill region needed

    const int MPX_ENABLE_BIT_NO = 0;
    const int BNDPRESERVE_BIT_NO = 1;

    const int bndpreserve = 1; // don't overwrite bounds during ext lib calls
    const int enable = 1;

    xsave_buf->bndcsr.cfg_reg_u |= enable << MPX_ENABLE_BIT_NO;
    xsave_buf->bndcsr.cfg_reg_u |= bndpreserve << BNDPRESERVE_BIT_NO;
    xsave_buf->bndcsr.status_reg = 0;

    mpx_xrstor_state(xsave_buf, 0x10);

    return true;
}

static void set_manual_mpx_bound(uint64_t bound) {
  const uint64_t lower = 0;

  asm volatile(
    "bndmk (%[lower], %[bound]), %%bnd1\n"
    :
    : [lower] "r"(lower), [bound] "r"(bound)
    :);
}

#endif


#if WASM_RT_USE_SEGUE || WASM_RT_USE_SHADOW_SEGUE || WASM_RT_USE_SEGUE_LOAD || WASM_RT_USE_SEGUE_STORE
void wasm_rt_set_segment_base(uintptr_t base) {
  _writegsbase_u64((uintptr_t)base);
}
#endif

#if WASM_RT_USE_MTE
/*
 * Insert a random logical tag into the given pointer.
 * IRG instruction.
 */
static uint64_t insert_mte_tag(void* ptr) {
  uint64_t __val;
  asm("irg %0, %1" : "=r" (__val) : "r" (ptr));
  return __val;
}

static void set_mte_tag_double(unsigned char* tagged_addr) {
  asm volatile("st2g %0, [%0]" : : "r" (tagged_addr) : "memory");
}

static void set_mte_tag_range(unsigned char* tagged_addr, int64_t size) {
  if (size % 32 != 0) {
    printf("MTE size not divisible by 32\n");
    abort();
  }

  while(size > 0) {
    set_mte_tag_double(tagged_addr);
    tagged_addr += 32;
    size -= 32;
  };
}

#endif

void wasm_rt_allocate_memory(wasm_rt_memory_t* memory,
                             uint64_t initial_pages,
                             uint64_t max_pages,
                             bool is64) {
  uint64_t byte_length = initial_pages * PAGE_SIZE;
  memory->size = byte_length;
  memory->pages = initial_pages;
  memory->max_pages = max_pages;
  memory->is64 = is64;

#if WASM_RT_USE_MMAP

#if WASM_RT_MEMCHECK_PRESHADOW_BYTES
  const size_t required_shadow_memory =
      get_required_shadow_bytes(memory->max_pages);
  const uint64_t nearest_page_count =
      div_and_roundup(required_shadow_memory, OSPAGE_SIZE);
  // create a memory allocation of "nearest_page_count" of accessible pages
  // followed by "nearest_page_count" of inaccessible pages
  const uint64_t disallowed_and_allowed_pages = nearest_page_count * 2;
  const uint64_t pre_shadow_bytes = disallowed_and_allowed_pages * OSPAGE_SIZE;
#elif WASM_RT_MEMCHECK_PRESHADOW_PAGE
  // 1 OS page per 256mb (2^28) of wasm memory.
  // 1 OS page per 2^28 / WASM_PAGE_SIZE
  //             = 2^28 / 2^16
  //             = 1 OS page per 2 ^ 12 Wasm pages
  const uint64_t page_count = div_and_roundup(max_pages, (1 << 12));
  const uint64_t pre_shadow_bytes = page_count * OSPAGE_SIZE;
#else
  const uint64_t pre_shadow_bytes = 0;
#endif

  const uint64_t mmap_size = get_allocation_size_for_mmap(memory);
  void* addr = os_mmap(pre_shadow_bytes + mmap_size);
  if (!addr) {
    os_print_last_error("os_mmap failed.");
    abort();
  }

#if WASM_RT_USE_MTE
  addr = insert_mte_tag((unsigned char *)addr);
#endif

#if WASM_RT_MEMCHECK_PRESHADOW_BYTES
  memory->shadow_bytes_allocation = addr;
  memory->shadow_bytes_allocation_size = pre_shadow_bytes;
  int ret_shadow = os_mprotect(addr, pre_shadow_bytes / 2);
  if (ret_shadow != 0) {
    os_print_last_error("os_mprotect preshadow failed.");
    abort();
  }
#elif WASM_RT_MEMCHECK_PRESHADOW_PAGE
  const uint64_t readable = div_and_roundup(initial_pages, 1 << 12)  * OSPAGE_SIZE;
  void* start_readable = ((char*) addr) + pre_shadow_bytes - readable;
  int ret_shadow = os_mprotect_read(start_readable, readable);
  if (ret_shadow != 0) {
    os_print_last_error("os_mprotect preshadow page failed.");
    abort();
  }
#endif
  addr = ((uint8_t*)addr) + pre_shadow_bytes;
  int ret = os_mprotect(addr, byte_length);
  if (ret != 0) {
    os_print_last_error("os_mprotect failed.");
    abort();
  }
  memory->data = addr;

#if WASM_RT_USE_MTE
  set_mte_tag_range(addr, byte_length);
#endif

#if WASM_RT_MEMCHECK_PRESHADOW_BYTES
  memory->shadow_bytes = get_shadow_bytes_pointer(
      memory->shadow_bytes_allocation, memory->shadow_bytes_allocation_size,
      initial_pages);
  memory->shadow_bytes_distance_from_heap = memory->data - memory->shadow_bytes;
#endif

#else
  memory->data = calloc(byte_length, 1);
#endif

#if WASM_RT_USE_SEGUE || WASM_RT_USE_SEGUE_LOAD || WASM_RT_USE_SEGUE_STORE || \
    (WASM_RT_MEMCHECK_PRESHADOW_BYTES && WASM_RT_USE_SHADOW_SEGUE) || \
    (WASM_RT_MEMCHECK_PRESHADOW_PAGE && WASM_RT_USE_SHADOW_SEGUE)
  _writegsbase_u64((uintptr_t)memory->data);
#endif

#if WASM_RT_MEMCHECK_SHADOW_PAGE
  const uint64_t shadow_memory_size = max_pages * OSPAGE_SIZE;
  void* shadow_memory = os_mmap(shadow_memory_size);
  if (!shadow_memory) {
    os_print_last_error("os_mmap shadow failed.");
    abort();
  }

#if WASM_RT_MEMCHECK_SHADOW_PAGE_SCHEME != 5
  uint64_t shadow_byte_length = initial_pages * OSPAGE_SIZE;
#else
  uint64_t shadow_byte_length = div_and_roundup(initial_pages, 1 << 24)  * OSPAGE_SIZE;
#endif

#if WASM_RT_MEMCHECK_SHADOW_PAGE_SCHEME == 3 || \
    WASM_RT_MEMCHECK_SHADOW_PAGE_SCHEME == 4
  int shadow_ret = os_mprotect(shadow_memory, shadow_byte_length);
#else
  int shadow_ret = os_mprotect_read(shadow_memory, shadow_byte_length);
#endif
  if (shadow_ret != 0) {
    os_print_last_error("os_mprotect shadow failed.");
    abort();
  }

  memory->shadow_memory = shadow_memory;

#if WASM_RT_USE_SHADOW_SEGUE
  _writegsbase_u64((uintptr_t)shadow_memory);
#endif

#endif

#if WASM_RT_MEMCHECK_SHADOW_BYTES_TAG
  const size_t required_shadow_memory =
      get_required_shadow_bytes(memory->max_pages);
  memory->shadow_bytes = calloc(required_shadow_memory, 1);

  const size_t accessible_bytes = get_required_shadow_bytes(memory->pages);

#if WASM_RT_MEMCHECK_SHADOW_BYTES_TAG_SCHEME == 1 || WASM_RT_MEMCHECK_SHADOW_BYTES_TAG_SCHEME == 2
  for(size_t i = accessible_bytes/sizeof(memory->shadow_bytes[0]); i < required_shadow_memory; i++) {
    uint32_t floatzone_x = 0x0b8b8b8a;
    memcpy(&(memory->shadow_bytes[i]), &floatzone_x, sizeof(floatzone_x));
  }
#endif

#if WASM_RT_USE_SHADOW_SEGUE
  _writegsbase_u64((uintptr_t)memory->shadow_bytes);
#endif

#endif

#if WASM_RT_MEMCHECK_SHADOW_BYTES
  const size_t required_shadow_memory =
      get_required_shadow_bytes(memory->max_pages);
  const uint64_t nearest_page_count =
      div_and_roundup(required_shadow_memory, OSPAGE_SIZE);
  // create a memory allocation of "nearest_page_count" of accessible pages
  // followed by "nearest_page_count" of inaccessible pages
  const uint64_t allowed_and_disallowed_pages = nearest_page_count * 2;
  void* shadow_bytes = os_mmap(allowed_and_disallowed_pages * OSPAGE_SIZE);
  if (!shadow_bytes) {
    os_print_last_error("os_mmap shadow_bytes failed.");
    abort();
  }

  memory->shadow_bytes_allocation = shadow_bytes;
  memory->shadow_bytes_allocation_size =
      allowed_and_disallowed_pages * OSPAGE_SIZE;
  const size_t accessible_size = memory->shadow_bytes_allocation_size / 2;

#if WASM_RT_MEMCHECK_SHADOW_BYTES_SCHEME <= 4
  int shadow_ret = os_mprotect_read(shadow_bytes, accessible_size);
#else
  int shadow_ret = os_mprotect(shadow_bytes, accessible_size);
#endif
  if (shadow_ret != 0) {
    os_print_last_error("os_mprotect shadow failed.");
    abort();
  }

  memory->shadow_bytes = get_shadow_bytes_pointer(
      memory->shadow_bytes_allocation, memory->shadow_bytes_allocation_size,
      initial_pages);

#if WASM_RT_USE_SHADOW_SEGUE
  _writegsbase_u64((uintptr_t)memory->shadow_bytes);
#endif
#endif

#if WASM_RT_MEMCHECK_DEBUG_WATCH
  memory->debug_watch_buffer = 0;
  // TODO: Add debug register watchpoints

#if WASM_RT_USE_SHADOW_SEGUE
  _writegsbase_u64((uintptr_t) & (memory->debug_watch_buffer));
#endif
#endif

#if WASM_RT_MEMCHECK_MPX
  enable_manual_mpx();
  set_manual_mpx_bound(memory->size);
#endif
}

static uint64_t grow_memory_impl(wasm_rt_memory_t* memory, uint64_t delta) {
  uint64_t old_pages = memory->pages;
  uint64_t new_pages = memory->pages + delta;
  if (new_pages == 0) {
    return 0;
  }
  if (new_pages < old_pages || new_pages > memory->max_pages) {
    return (uint64_t)-1;
  }
  uint64_t old_size = old_pages * PAGE_SIZE;
  uint64_t new_size = new_pages * PAGE_SIZE;
  uint64_t delta_size = delta * PAGE_SIZE;
#if WASM_RT_USE_MMAP
  uint8_t* new_data = memory->data;
  int ret = os_mprotect(new_data + old_size, delta_size);
  if (ret != 0) {
    return (uint64_t)-1;
  }
#if WASM_RT_USE_MTE
  set_mte_tag_range(new_data + old_size, delta_size);
#endif

#else
  uint8_t* new_data = realloc(memory->data, new_size);
  if (new_data == NULL) {
    return (uint64_t)-1;
  }
#if !WABT_BIG_ENDIAN
  memset(new_data + old_size, 0, delta_size);
#endif
#endif

#if WASM_RT_MEMCHECK_SHADOW_PAGE

#if WASM_RT_MEMCHECK_SHADOW_PAGE_SCHEME != 5
  uint64_t shadow_old_size = old_pages * OSPAGE_SIZE;
  uint64_t shadow_new_size = new_pages * OSPAGE_SIZE;
  uint64_t shadow_delta_size = delta * OSPAGE_SIZE;
#else
  uint64_t shadow_old_size = div_and_roundup(old_pages, 1 << 24)  * OSPAGE_SIZE;
  uint64_t shadow_new_size = div_and_roundup(new_pages, 1 << 24)  * OSPAGE_SIZE;
  uint64_t shadow_delta_size = div_and_roundup(delta, 1 << 24)  * OSPAGE_SIZE;
#endif

  int shadow_ret = 1;

  if (shadow_delta_size != 0) {
#if WASM_RT_MEMCHECK_SHADOW_PAGE_SCHEME == 3 || \
    WASM_RT_MEMCHECK_SHADOW_PAGE_SCHEME == 4
    shadow_ret =
        os_mprotect(memory->shadow_memory + shadow_old_size, shadow_delta_size);
#else
    shadow_ret = os_mprotect_read(memory->shadow_memory + shadow_old_size,
                                    shadow_delta_size);
#endif
  }

  if (shadow_ret != 0) {
    return (uint64_t)-1;
  }
#endif


#if WASM_RT_MEMCHECK_SHADOW_BYTES_TAG
  const size_t old_accessible_bytes = get_required_shadow_bytes(old_pages);
  const size_t old_accessible_index = old_accessible_bytes/sizeof(memory->shadow_bytes[0]);
  const size_t new_accessible_bytes = get_required_shadow_bytes(new_pages);

#if WASM_RT_MEMCHECK_SHADOW_BYTES_TAG_SCHEME == 1 || WASM_RT_MEMCHECK_SHADOW_BYTES_TAG_SCHEME == 2
  memset(&(memory->shadow_bytes[old_accessible_index]), 0, new_accessible_bytes - old_accessible_bytes);
#endif

#elif WASM_RT_MEMCHECK_SHADOW_BYTES
  memory->shadow_bytes =
      get_shadow_bytes_pointer(memory->shadow_bytes_allocation,
                               memory->shadow_bytes_allocation_size, new_pages);
#if WASM_RT_USE_SHADOW_SEGUE
  _writegsbase_u64((uintptr_t)memory->shadow_bytes);
#endif

#elif WASM_RT_MEMCHECK_PRESHADOW_BYTES
  memory->shadow_bytes =
      get_shadow_bytes_pointer(memory->shadow_bytes_allocation,
                               memory->shadow_bytes_allocation_size, new_pages);
  memory->shadow_bytes_distance_from_heap = memory->data - memory->shadow_bytes;
#endif

#if WASM_RT_MEMCHECK_PRESHADOW_PAGE
  uint64_t shadow_old_size = div_and_roundup(old_pages, 1 << 12)  * OSPAGE_SIZE;
  uint64_t shadow_new_size = div_and_roundup(new_pages, 1 << 12)  * OSPAGE_SIZE;
  uint64_t shadow_delta_size = div_and_roundup(delta, 1 << 12)  * OSPAGE_SIZE;

  void* start_readable = ((char*) memory->data) - shadow_new_size;
  int shadow_ret = os_mprotect_read(start_readable, shadow_delta_size);

  if (shadow_ret != 0) {
    return (uint64_t)-1;
  }
#endif

  // if WASM_RT_MEMCHECK_DEBUG_WATCH adjust debug watch

#if WABT_BIG_ENDIAN
  memmove(new_data + new_size - old_size, new_data, old_size);
  memset(new_data, 0, delta_size);
#endif

#if WASM_RT_MEMCHECK_MPX
  enable_manual_mpx();
  set_manual_mpx_bound(new_size);
#endif

  memory->pages = new_pages;
  memory->size = new_size;
  memory->data = new_data;
  return old_pages;
}

uint64_t wasm_rt_grow_memory(wasm_rt_memory_t* memory, uint64_t delta) {
  uint64_t ret = grow_memory_impl(memory, delta);
#ifdef WASM_RT_GROW_FAILED_HANDLER
  if (ret == -1) {
    WASM_RT_GROW_FAILED_HANDLER();
  }
#endif
  return ret;
}

void wasm_rt_free_memory(wasm_rt_memory_t* memory) {
#if WASM_RT_USE_MMAP

#if WASM_RT_MEMCHECK_PRESHADOW_BYTES
  void* addr = memory->shadow_bytes_allocation;
  const uint64_t pre_shadow_bytes = memory->shadow_bytes_allocation_size;
#else
  void* addr = memory->data;
  const uint64_t pre_shadow_bytes = 0;
#endif

  const uint64_t mmap_size = get_allocation_size_for_mmap(memory);
  os_munmap(addr, pre_shadow_bytes + mmap_size);  // ignore error
#else
  free(memory->data);
#endif

#if WASM_RT_MEMCHECK_SHADOW_PAGE
  const uint64_t shadow_memory_size = memory->max_pages * OSPAGE_SIZE;
  os_munmap(memory->shadow_memory, shadow_memory_size);  // ignore error
#endif

#if WASM_RT_MEMCHECK_SHADOW_BYTES_TAG
  free(memory->shadow_bytes);
#endif
#if WASM_RT_MEMCHECK_SHADOW_BYTES
  os_munmap(memory->shadow_bytes_allocation,
            memory->shadow_bytes_allocation_size);  // ignore error
#endif
}

#define DEFINE_TABLE_OPS(type)                                          \
  void wasm_rt_allocate_##type##_table(wasm_rt_##type##_table_t* table, \
                                       uint32_t elements,               \
                                       uint32_t max_elements) {         \
    table->size = elements;                                             \
    table->max_size = max_elements;                                     \
    table->data = calloc(table->size, sizeof(wasm_rt_##type##_t));      \
  }                                                                     \
  void wasm_rt_free_##type##_table(wasm_rt_##type##_table_t* table) {   \
    free(table->data);                                                  \
  }                                                                     \
  uint32_t wasm_rt_grow_##type##_table(wasm_rt_##type##_table_t* table, \
                                       uint32_t delta,                  \
                                       wasm_rt_##type##_t init) {       \
    uint32_t old_elems = table->size;                                   \
    uint64_t new_elems = (uint64_t)table->size + delta;                 \
    if (new_elems == 0) {                                               \
      return 0;                                                         \
    }                                                                   \
    if ((new_elems < old_elems) || (new_elems > table->max_size)) {     \
      return (uint32_t)-1;                                              \
    }                                                                   \
    void* new_data =                                                    \
        realloc(table->data, new_elems * sizeof(wasm_rt_##type##_t));   \
    if (!new_data) {                                                    \
      return (uint32_t)-1;                                              \
    }                                                                   \
    table->data = new_data;                                             \
    table->size = new_elems;                                            \
    for (uint32_t i = old_elems; i < new_elems; i++) {                  \
      table->data[i] = init;                                            \
    }                                                                   \
    return old_elems;                                                   \
  }

DEFINE_TABLE_OPS(funcref)
DEFINE_TABLE_OPS(externref)

const char* wasm_rt_strerror(wasm_rt_trap_t trap) {
  switch (trap) {
    case WASM_RT_TRAP_NONE:
      return "No error";
    case WASM_RT_TRAP_OOB:
#if WASM_RT_MERGED_OOB_AND_EXHAUSTION_TRAPS
      return "Out-of-bounds access in linear memory or a table, or call stack "
             "exhausted";
#else
      return "Out-of-bounds access in linear memory or a table";
    case WASM_RT_TRAP_EXHAUSTION:
      return "Call stack exhausted";
#endif
    case WASM_RT_TRAP_INT_OVERFLOW:
      return "Integer overflow on divide or truncation";
    case WASM_RT_TRAP_DIV_BY_ZERO:
      return "Integer divide by zero";
    case WASM_RT_TRAP_INVALID_CONVERSION:
      return "Conversion from NaN to integer";
    case WASM_RT_TRAP_UNREACHABLE:
      return "Unreachable instruction executed";
    case WASM_RT_TRAP_CALL_INDIRECT:
      return "Invalid call_indirect";
    case WASM_RT_TRAP_UNCAUGHT_EXCEPTION:
      return "Uncaught exception";
    case WASM_RT_TRAP_UNALIGNED:
      return "Unaligned atomic memory access";
  }
  return "invalid trap code";
}
