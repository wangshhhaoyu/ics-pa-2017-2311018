#include "monitor/watchpoint.h"
#include "monitor/expr.h"

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

/* TODO: Implement the functionality of watchpoint */

WP *new_wp()
{
  if (free_ == NULL)
  {
    assert(0);
    return NULL;
  }

  WP *wp = free_;
  free_ = free_->next;

  wp->next = head;
  head = wp;

  wp->enabled = true;
  return wp;
}

void free_wp(WP *wp)
{
  if (wp == NULL)
    return;

  WP **pp = &head;
  while (*pp != NULL && *pp != wp)
  {
    pp = &(*pp)->next;
  }

  if (*pp == wp)
  {
    *pp = wp->next;
  }

  wp->expr[0] = '\0';
  wp->old_val = 0;
  wp->enabled = false;
  wp->next = free_;
  free_ = wp;
}

void print_watchpoints()
{
  if (head == NULL)
  {
    printf("No watchpoints.\n");
    return;
  }

  printf("Num     Type           Disp Enb Address    What\n");
  WP *wp = head;
  while (wp != NULL)
  {
    printf("%-8dwatchpoint     keep y   %-10s %s\n",
           wp->NO, "", wp->expr);
    wp = wp->next;
  }
}

bool check_watchpoints()
{
  WP *wp = head;
  bool triggered = false;

  while (wp != NULL)
  {
    if (wp->enabled && wp->expr[0] != '\0')
    {
      bool success = true;
      uint32_t new_val = expr(wp->expr, &success);

      if (!success)
      {
        printf("Warning: Failed to evaluate watchpoint %d expression: %s\n",
               wp->NO, wp->expr);
        wp = wp->next;
        continue;
      }

      if (new_val != wp->old_val)
      {
        printf("Watchpoint %d: %s\n\n", wp->NO, wp->expr);
        printf("Old value = 0x%08x (%d)\n", wp->old_val, (int32_t)wp->old_val);
        printf("New value = 0x%08x (%d)\n", new_val, (int32_t)new_val);

        wp->old_val = new_val;
        triggered = true;
      }
    }
    wp = wp->next;
  }

  return triggered;
}

void delete_watchpoint(int no)
{
  WP *wp = head;
  while (wp != NULL)
  {
    if (wp->NO == no)
    {
      free_wp(wp);
      printf("Watchpoint %d deleted.\n", no);
      return;
    }
    wp = wp->next;
  }
  printf("No watchpoint number %d.\n", no);
}
