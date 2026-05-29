#ifndef __WATCHPOINT_H__
#define __WATCHPOINT_H__

#include "common.h"

typedef struct watchpoint {
  int NO;
  struct watchpoint *next;

  /* TODO: Add more members if necessary */
  char expr[256];
  uint32_t old_val;
  bool enabled;

} WP;

void init_wp_pool();
WP *new_wp();
void free_wp(WP *wp);
void print_watchpoints();
bool check_watchpoints();
void delete_watchpoint(int no);

#endif
