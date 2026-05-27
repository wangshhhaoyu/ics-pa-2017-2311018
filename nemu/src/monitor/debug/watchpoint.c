#include "monitor/watchpoint.h"
#include "monitor/expr.h"
#include <string.h>

#define NR_WP 32

static WP wp_pool[NR_WP];
static WP *head, *free_;

void init_wp_pool() {
  int i;
  for (i = 0; i < NR_WP; i ++) {
    wp_pool[i].NO = i;
    wp_pool[i].next = &wp_pool[i + 1];
  }
  wp_pool[NR_WP - 1].next = NULL;

  head = NULL;
  free_ = wp_pool;
}

WP *new_wp(char *e) {
  if (free_ == NULL) {
    printf("No free watchpoint!\n");
    return NULL;
  }

  WP *wp = free_;
  free_ = free_->next;

  /* Get initial value */
  bool success;
  wp->old_val = expr(e, &success);
  if (!success) {
    printf("Invalid expression: %s\n", e);
    free_->next = free_;
    free_ = wp;
    return NULL;
  }

  strncpy(wp->expr, e, sizeof(wp->expr) - 1);
  wp->expr[sizeof(wp->expr) - 1] = '\0';
  wp->hit = false;
  wp->next = head;
  head = wp;

  printf("Watchpoint %d: %s\n", wp->NO, e);
  printf("  Old value = 0x%08x\n", wp->old_val);

  return wp;
}

void free_wp(int no) {
  WP *prev = NULL;
  WP *p = head;

  while (p != NULL) {
    if (p->NO == no) {
      if (prev == NULL) {
        head = p->next;
      } else {
        prev->next = p->next;
      }

      p->next = free_;
      free_ = p;

      printf("Watchpoint %d deleted\n", no);
      return;
    }

    prev = p;
    p = p->next;
  }

  printf("Watchpoint %d not found\n", no);
}

void info_wp() {
  if (head == NULL) {
    printf("No watchpoints.\n");
    return;
  }

  WP *p = head;
  printf("%-5s %-20s %-15s\n", "NO", "Expr", "Value");
  printf("----------------------------------------\n");

  while (p != NULL) {
    printf("%-5d %-20s 0x%08x\n", p->NO, p->expr, p->old_val);
    p = p->next;
  }
}

bool check_wp() {
  WP *p = head;
  bool changed = false;

  while (p != NULL) {
    bool success;
    uint32_t new_val = expr(p->expr, &success);

    if (success && new_val != p->old_val) {
      printf("\n");
      printf("Watchpoint %d: %s\n", p->NO, p->expr);
      printf("  Old value = 0x%08x\n", p->old_val);
      printf("  New value = 0x%08x\n", new_val);
      p->old_val = new_val;
      changed = true;
    }

    p = p->next;
  }

  return changed;
}


