#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#ifndef INTERVAL_TREE_IMPL_H
#define INTERVAL_TREE_IMPL_H

#define INTERVAL_TREE_SIZE 64
struct interval_tree {
  uint64_t curr;
  uint64_t start[INTERVAL_TREE_SIZE];
  uint64_t end[INTERVAL_TREE_SIZE];
};

void add_interval(struct interval_tree* d, uint64_t start, uint64_t end) {
  if (d->curr >= INTERVAL_TREE_SIZE) {
    printf("Out of space in interval tree");
    abort();
  }

  d->start[d->curr] = start;
  d->end[d->curr] = end;
  d->curr++;
}

int is_in_interval(struct interval_tree* d, uint64_t val) {
  for (uint64_t i = 0; i < d->curr; i++) {
    if(val >= d->start[i] && val <= d->end[i]) {
      return 1;
    }
  }
  return 0;
}

int is_in_interval2(struct interval_tree* d, uint64_t start, uint64_t end) {
  for (uint64_t i = 0; i < d->curr; i++) {
    if(end >= d->start[i] && start <= d->end[i]) {
      return 1;
    }
  }
  return 0;
}

void for_each_interval(struct interval_tree* d, void* param, void (*cb)(void* p, uint64_t start, uint64_t end)) {
  for (uint64_t i = 0; i < d->curr; i++) {
    cb(param, d->start[i], d->end[i]);
  }
}

#endif
