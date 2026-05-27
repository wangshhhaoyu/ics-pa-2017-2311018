#ifndef __WATCHPOINT_H__
#define __WATCHPOINT_H__

#include "common.h"

#define NR_WP 32

typedef struct watchpoint {
  int NO;
  struct watchpoint *next;

  /* TODO: Add more members if necessary */
  char expr[128];
  uint32_t old_val;
  bool hit;

} WP;

void init_wp_pool();
WP *new_wp(char *e);
void free_wp(int no);
void info_wp();
bool check_wp();

#endif
