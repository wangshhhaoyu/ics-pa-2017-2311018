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

static int cmd_si(char *args) {
  char *arg = strtok(NULL, " ");
  if (arg == NULL) {
    cpu_exec(1);
  } else {
    int n = atoi(arg);
    if (n <= 0) {
      printf("Invalid number: %s\n", arg);
      return 0;
    }
    cpu_exec(n);
  }
  return 0;
}

static int cmd_info(char *args) {
  char *arg = strtok(NULL, " ");
  if (arg == NULL) {
    printf("Usage: info r | info w\n");
    return 0;
  }

  if (strcmp(arg, "r") == 0) {
    int i;
    for (i = 0; i < 8; i++) {
      printf("%-3s\t0x%08x\t%d\n", regsl[i], cpu.gpr[i]._32, cpu.gpr[i]._32);
    }
    printf("eip\t0x%08x\t%d\n", cpu.eip, cpu.eip);
  } else if (strcmp(arg, "w") == 0) {
    // TODO: implement info watchpoints
    printf("TODO: implement info watchpoints\n");
  } else {
    printf("Unknown argument: %s\n", arg);
  }
  return 0;
}

static int cmd_p(char *args) {
  if (args == NULL || *args == '\0') {
    printf("Usage: p EXPR\n");
    return 0;
  }

  bool success;
  uint32_t result = expr(args, &success);

  if (success) {
    printf("0x%08x (%u)\n", result, result);
  } else {
    printf("Invalid expression\n");
  }

  return 0;
}

static int cmd_x(char *args) {
  char *n_str = strtok(NULL, " ");
  char *expr_str = strtok(NULL, " ");

  if (n_str == NULL || expr_str == NULL) {
    printf("Usage: x N EXPR\n");
    return 0;
  }

  int n = atoi(n_str);
  if (n <= 0) {
    printf("Invalid number: %s\n", n_str);
    return 0;
  }

  bool success;
  vaddr_t addr = expr(expr_str, &success);

  if (!success) {
    printf("Invalid expression\n");
    return 0;
  }

  printf("0x%08x: ", addr);
  int i;
  for (i = 0; i < n; i++) {
    if (i > 0 && i % 4 == 0) {
      printf("\n0x%08x: ", addr + i * 4);
    }
    uint32_t data = vaddr_read(addr + i * 4, 4);
    printf("%08x ", data);
  }
  printf("\n");

  return 0;
}

static int cmd_w(char *args) {
  if (args == NULL || *args == '\0') {
    printf("Usage: w EXPR\n");
    return 0;
  }

  bool success;
  uint32_t result = expr(args, &success);

  if (!success) {
    printf("Invalid expression\n");
    return 0;
  }

  // TODO: implement watchpoint
  printf("TODO: set watchpoint at 0x%08x\n", result);

  return 0;
}

static int cmd_d(char *args) {
  if (args == NULL || *args == '\0') {
    printf("Usage: d N\n");
    return 0;
  }

  int n = atoi(args);
  if (n <= 0) {
    printf("Invalid number: %s\n", args);
    return 0;
  }

  // TODO: implement delete watchpoint
  printf("TODO: delete watchpoint %d\n", n);

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
  { "si", "Single step execution", cmd_si },
  { "info", "Display information about registers or watchpoints", cmd_info },
  { "p", "Evaluate expression", cmd_p },
  { "x", "Examine memory", cmd_x },
  { "w", "Set watchpoint", cmd_w },
  { "d", "Delete watchpoint", cmd_d },

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

#ifdef HAS_IOE
    extern void sdl_clear_event_queue(void);
    sdl_clear_event_queue();
#endif

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
