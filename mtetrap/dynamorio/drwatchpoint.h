#ifndef DRWATCHPOINT_H
#define DRWATCHPOINT_H

//////////////////////////////// dynamorio-based recovery ////////////////////////////////

#include "../interval_tree.h"
#include <string.h>
#include <stdint.h>
#include <stddef.h>

int dr_instrument_enable = 0;

void install_dr_based_recovery() {
  dr_instrument_enable = 1;
}

void uninstall_dr_based_recovery() {
  dr_instrument_enable = 0;
}

struct interval_tree dr_watched = {0};
uint64_t dr_access_count = 0;
uint64_t dr_hit_count = 0;

void add_dr_data_watchpoint(unsigned char * ptr, size_t length) {
  const uint64_t ptr_end = (uint64_t) (ptr + length - 1);
  add_interval(&dr_watched, (uint64_t)ptr, ptr_end);
  printf("add_interval(%p, %lu)\n", ptr, length);
}

void remove_all_dr_data_watchpoints() {
  printf("remove_all_dr_data_watchpoints() : curr %lu\n", dr_watched.curr);
  memset(&dr_watched, 0, sizeof(dr_watched));
  printf("Accessed %lu/%lu times\n\n", dr_hit_count, dr_access_count);
  dr_access_count = 0;
  dr_hit_count = 0;
}
#endif
