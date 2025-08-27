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

#include "mtetrap_recovery.h"

int main(void)
{
  pid_t mypid = getpid();
  printf("Process pid: %lu\n", (unsigned long) mypid);
  unsigned char * ptr = mmap(NULL, sysconf(_SC_PAGESIZE), PROT_READ | PROT_WRITE | PROT_MTE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (ptr == MAP_FAILED) {
    perror("mmap() failed");
    return EXIT_FAILURE;
  }

  add_mte_data_watchpoint(ptr, 1);

  {
    printf("Running: test for signal based recovery\n");
    install_signal_based_recovery();

    printf("Running the faulting instruction...\n");
    ptr[0] = 0x55;

    printf("Success: Executing past the faulting instruction\n");
    uninstall_signal_based_recovery();

    printf("Accessed %lu times\n\n", mte_access_count);
  }

  {
    printf("Running: test for module based recovery\n");
    install_module_based_recovery();

    printf("Running the faulting instruction...\n");
    ptr[0] = 0x55;

    printf("Success: Executing past the faulting instruction\n\n");
    uninstall_module_based_recovery();

    printf("Accessed %lu times\n\n", mte_access_count);
  }

  remove_all_mte_data_watchpoints();

  add_mprotect_data_watchpoint(ptr, 1);

  {
    printf("Running: test for mprotect based recovery\n");
    install_mprotect_based_recovery();

    printf("Running the faulting instruction...\n");
    ptr[0] = 0x55;

    printf("Success: Executing past the faulting instruction\n\n");
    uninstall_mprotect_based_recovery();

    printf("Accessed %lu times\n\n", mprotect_access_count);
  }

  remove_all_mprotect_data_watchpoints();

  return 0;
}
