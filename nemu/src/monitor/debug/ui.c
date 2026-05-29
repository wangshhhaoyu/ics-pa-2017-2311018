#include "monitor/monitor.h"
#include "monitor/expr.h"
#include "monitor/watchpoint.h"
#include "nemu.h"

#include <stdlib.h>
#include <readline/readline.h>
#include <readline/history.h>

void cpu_exec(uint64_t);

/* We use the `readline' library to provide more flexibility to read from stdin. */
char* rl_gets() {
  static char *line_read = NULL;

  if (line_read) {
    free(line_read);
    line_read = NULL;
  }

  line_read = readline("(nemu) ");

  if (line_read && *line_read) {
    add_history(line_read);
  }

  return line_read;
}

static int cmd_c(char *args) {
  cpu_exec(-1);
  return 0;
}

static int cmd_q(char *args) {
  return -1;
}

static int cmd_expr(char *args)
{
  if (args == NULL)
  {
    printf("Usage: p <expression>\n");
    printf("Example: p 1+2 * 3  or  p $eax+4\n");
    return 0;
  }

  bool success;
  uint32_t result = expr(args, &success);
  if (success)
  {
    printf("Result: 0x%08x (%d)\n", result, (int32_t)result);
  }
  else
  {
    printf("Expression evaluation failed.\n");
  }
  return 0;
}

static int cmd_si(char *args)
{
  int step = 1;
  if (args != NULL)
  {
    bool success = false;
    uint32_t step_val = expr(args, &success);

    if (success)
    {
      step = (int)step_val;
      if (step <= 0)
      {
        printf("Step count must be positive (got %d)\n", step);
        return 0;
      }
    }
    else
    {
      printf("Failed to evaluate step expression: %s\n", args);
      return 0;
    }
  }

  cpu_exec(step);
  return 0;
}

void isa_reg_display(void);

static int cmd_w(char *args)
{
  if (args == NULL)
  {
    printf("Usage: w <expression>\n");
    printf("Set a watchpoint for an expression.\n");
    return 0;
  }

  bool success;
  uint32_t val = expr(args, &success);
  if (!success)
  {
    printf("Invalid expression: %s\n", args);
    return 0;
  }

  WP *wp = new_wp();
  if (wp == NULL)
  {
    printf("Failed to create watchpoint: no free watchpoint available.\n");
    return 0;
  }

  strncpy(wp->expr, args, sizeof(wp->expr) - 1);
  wp->expr[sizeof(wp->expr) - 1] = '\0';
  wp->old_val = val;

  printf("Watchpoint %d: %s\n", wp->NO, wp->expr);
  return 0;
}

static int cmd_d(char *args)
{
  if (args == NULL)
  {
    printf("Usage: d <watchpoint_number>\n");
    printf("Delete a watchpoint.\n");
    return 0;
  }

  int no = atoi(args);
  delete_watchpoint(no);
  return 0;
}

static int cmd_info(char *args)
{
  if (args == NULL)
  {
    printf("Usage: info <subcommand>\n");
    printf("Subcommands:\n");
    printf("  r - print register values\n");
    printf("  w - print watchpoints (to be implemented)\n");
    return 0;
  }
  char *subcmd = strtok(args, " ");
  if (subcmd == NULL)
  {
    printf("Please specify a subcommand: r or w\n");
    return 0;
  }
  if (strcmp(subcmd, "r") == 0)
  {
    isa_reg_display();
  }
  else if (strcmp(subcmd, "w") == 0)
  {
    print_watchpoints();
  }
  else
  {
    printf("Unknown subcommand '%s'\n", subcmd);
  }
  return 0;
}

static int cmd_x(char *args)
{
  if (args == NULL)
  {
    printf("Usage: x <count_expression> <address_expression>\n");
    printf("Both parameters support full expression evaluation.\n");
    printf("Examples:\n");
    printf("  x 10 0x100000          # 查看固定地址\n");
    printf("  x 2 * 4 $eip             # 查看8字节，从eip开始\n");
    printf("  x $eax $esp+($ebx*2)  # 数量由eax决定，地址动态计算\n");
    return 0;
  }

  char *count_expr_end = args;
  while (*count_expr_end != '\0' && *count_expr_end != ' ')
  {
    count_expr_end++;
  }

  if (*count_expr_end == '\0')
  {
    printf("Missing address expression. Usage: x <count> <address>\n");
    return 0;
  }

  *count_expr_end = '\0';
  char *count_expr = args;
  char *addr_expr = count_expr_end + 1;

  bool success = false;
  uint32_t count = expr(count_expr, &success);
  if (!success)
  {
    printf("Failed to evaluate count expression: %s\n", count_expr);
    *count_expr_end = ' ';
    return 0;
  }

  if (count <= 0 || count > 1024)
  {
    printf("Count must be between 1 and 1024 (got %u)\n", count);
    *count_expr_end = ' ';
    return 0;
  }

  uint32_t addr = expr(addr_expr, &success);
  if (!success)
  {
    printf("Failed to evaluate address expression: %s\n", addr_expr);
    *count_expr_end = ' ';
    return 0;
  }

  printf("Scanning %u bytes from 0x%08x\n", count, addr);
  printf("  Count expression: %s\n", count_expr);
  printf("  Addr expression:  %s\n", addr_expr);
  printf("\n");
  printf("Address       +0    +1    +2    +3    +4    +5    +6    +7\n");
  printf("==========   ====  ====  ====  ====  ====  ====  ====  ====\n");

  *count_expr_end = ' ';

  for (uint32_t i = 0; i < count; i += 8)
  {
    printf("0x%08x  ", addr + i);
    for (int j = 0; j < 8 && (i + j) < count; j++)
    {
      uint8_t byte = paddr_read(addr + i + j, 1);
      printf("0x%02x  ", byte);
    }
    printf("\n");
  }
  return 0;
}

static int cmd_help(char *args);

static struct {
  char *name;
  char *description;
  int (*handler) (char *);
} cmd_table [] = {
  { "help", "Display informations about all supported commands", cmd_help },
  { "c", "Continue the execution of the program", cmd_c },
  { "q", "Exit NEMU", cmd_q },

  /* TODO: Add more commands */
  { "si", "Setp N(default 1)", cmd_si },
  { "info", "Print program status", cmd_info },
  { "x", "Scan memory", cmd_x },
  { "p", "Evaluate the expression", cmd_expr },
  { "w", "Set a watchpoint", cmd_w },
  { "d", "Delete a watchpoint", cmd_d },
};

#define NR_CMD (sizeof(cmd_table) / sizeof(cmd_table[0]))

static int cmd_help(char *args) {
  /* extract the first argument */
  char *arg = strtok(NULL, " ");
  int i;

  if (arg == NULL) {
    /* no argument given */
    for (i = 0; i < NR_CMD; i ++) {
      printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
    }
  }
  else {
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(arg, cmd_table[i].name) == 0) {
        printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
        return 0;
      }
    }
    printf("Unknown command '%s'\n", arg);
  }
  return 0;
}

void ui_mainloop(int is_batch_mode) {
  if (is_batch_mode) {
    cmd_c(NULL);
    return;
  }

  while (1) {
    char *str = rl_gets();
    char *str_end = str + strlen(str);

    /* extract the first token as the command */
    char *cmd = strtok(str, " ");
    if (cmd == NULL) { continue; }

    /* treat the remaining string as the arguments,
     * which may need further parsing
     */
    char *args = cmd + strlen(cmd) + 1;
    if (args >= str_end) {
      args = NULL;
    }


    int i;
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(cmd, cmd_table[i].name) == 0) {
        if (cmd_table[i].handler(args) < 0) { return; }
        break;
      }
    }

    if (i == NR_CMD) { printf("Unknown command '%s'\n", cmd); }
  }
}
