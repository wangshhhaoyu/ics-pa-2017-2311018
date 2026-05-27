#include "nemu.h"

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <sys/types.h>
#include <regex.h>
#include <ctype.h>
#include <stdlib.h>

enum {
  TK_NOTYPE = 256, TK_EQ,

  /* TODO: Add more token types */
  TK_NUM, TK_HEX, TK_REG, TK_DEREF, TK_NEG, TK_NEQ, TK_AND, TK_OR

};

static struct rule {
  char *regex;
  int token_type;
} rules[] = {

  /* TODO: Add more rules.
   * Pay attention to the precedence level of different rules.
   */

  {" +", TK_NOTYPE},    // spaces
  {"\\+", '+'},         // plus
  {"==", TK_EQ},         // equal
  {"!=", TK_NEQ},       // not equal
  {"&&", TK_AND},        // logical and
  {"\\|\\|", TK_OR},    // logical or
  {"!", '!'},            // logical not
  {"\\*", '*'},          // multiply or dereference
  {"/", '/'},            // divide
  {"-", '-'},            // minus or negative
  {"\\(", '('},          // left paren
  {"\\)", ')'},          // right paren
  {"0x[0-9a-fA-F]+", TK_HEX},  // hexadecimal number
  {"[0-9]+", TK_NUM},          // decimal number
  {"\\$[a-zA-Z]+", TK_REG},    // register

};

#define NR_REGEX (sizeof(rules) / sizeof(rules[0]) )

static regex_t re[NR_REGEX];

/* Rules are used for many times.
 * Therefore we compile them only once before any usage.
 */
void init_regex() {
  int i;
  char error_msg[128];
  int ret;

  for (i = 0; i < NR_REGEX; i ++) {
    ret = regcomp(&re[i], rules[i].regex, REG_EXTENDED);
    if (ret != 0) {
      regerror(ret, &re[i], error_msg, 128);
      panic("regex compilation failed: %s\n%s", error_msg, rules[i].regex);
    }
  }
}

typedef struct token {
  int type;
  char str[32];
  int priority;
} Token;

Token tokens[32];
int nr_token;

static bool make_token(char *e) {
  int position = 0;
  int i;
  regmatch_t pmatch;

  nr_token = 0;

  while (e[position] != '\0') {
    /* Try all rules one by one. */
    for (i = 0; i < NR_REGEX; i ++) {
      if (regexec(&re[i], e + position, 1, &pmatch, 0) == 0 && pmatch.rm_so == 0) {
        char *substr_start = e + position;
        int substr_len = pmatch.rm_eo;

        Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s",
            i, rules[i].regex, position, substr_len, substr_len, substr_start);
        position += substr_len;

        /* Record the token in the array `tokens'. For certain types
         * of tokens, some extra actions should be performed.
         */

        if (nr_token >= 32) {
          printf("Too many tokens!\n");
          return false;
        }

        switch (rules[i].token_type) {
          case TK_NOTYPE:
            break;
          case TK_NUM:
          case TK_HEX:
          case TK_REG:
            strncpy(tokens[nr_token].str, substr_start, substr_len);
            tokens[nr_token].str[substr_len] = '\0';
            tokens[nr_token].type = rules[i].token_type;
            tokens[nr_token].priority = 0;
            nr_token++;
            break;
          case '*':
          case '/':
            /* Check if * is dereference or multiply */
            if (rules[i].token_type == '*') {
              if (nr_token == 0 || tokens[nr_token - 1].type == '(' ||
                  tokens[nr_token - 1].type == '+' || tokens[nr_token - 1].type == '-' ||
                  tokens[nr_token - 1].type == '*' || tokens[nr_token - 1].type == '/' ||
                  tokens[nr_token - 1].type == TK_DEREF || tokens[nr_token - 1].type == TK_NEG) {
                tokens[nr_token].type = TK_DEREF;
                tokens[nr_token].priority = 7;
              } else {
                tokens[nr_token].type = '*';
                tokens[nr_token].priority = 4;
              }
            } else if (rules[i].token_type == '/') {
              tokens[nr_token].type = '/';
              tokens[nr_token].priority = 4;
            }
            nr_token++;
            break;
          case '-':
            /* Check if - is negative or minus */
            if (nr_token == 0 || tokens[nr_token - 1].type == '(' ||
                tokens[nr_token - 1].type == '+' || tokens[nr_token - 1].type == '-' ||
                tokens[nr_token - 1].type == '*' || tokens[nr_token - 1].type == '/' ||
                tokens[nr_token - 1].type == TK_DEREF || tokens[nr_token - 1].type == TK_NEG) {
              tokens[nr_token].type = TK_NEG;
              tokens[nr_token].priority = 7;
            } else {
              tokens[nr_token].type = '-';
              tokens[nr_token].priority = 3;
            }
            nr_token++;
            break;
          case '+':
            tokens[nr_token].type = '+';
            tokens[nr_token].priority = 3;
            nr_token++;
            break;
          case '(':
            tokens[nr_token].type = '(';
            tokens[nr_token].priority = 0;
            nr_token++;
            break;
          case ')':
            tokens[nr_token].type = ')';
            tokens[nr_token].priority = 0;
            nr_token++;
            break;
          case TK_EQ:
            tokens[nr_token].type = TK_EQ;
            tokens[nr_token].priority = 2;
            nr_token++;
            break;
          case TK_NEQ:
            tokens[nr_token].type = TK_NEQ;
            tokens[nr_token].priority = 2;
            nr_token++;
            break;
          case TK_AND:
            tokens[nr_token].type = TK_AND;
            tokens[nr_token].priority = 1;
            nr_token++;
            break;
          case TK_OR:
            tokens[nr_token].type = TK_OR;
            tokens[nr_token].priority = 0;
            nr_token++;
            break;
          case '!':
            tokens[nr_token].type = '!';
            tokens[nr_token].priority = 7;
            nr_token++;
            break;
          default:
            tokens[nr_token].type = rules[i].token_type;
            tokens[nr_token].priority = 0;
            nr_token++;
            break;
        }

        break;
      }
    }

    if (i == NR_REGEX) {
      printf("no match at position %d\n%s\n%*.s^\n", position, e, position, "");
      return false;
    }
  }

  return true;
}

/* Check if parentheses are balanced */
static bool check_parentheses(int p, int q) {
  int i, balance = 0;
  for (i = p; i <= q; i++) {
    if (tokens[i].type == '(') balance++;
    else if (tokens[i].type == ')') balance--;
    if (balance < 0) return false;
  }
  return balance == 0;
}

/* Find the dominant operator */
static int find_dominant_operator(int p, int q) {
  int i;
  int pos = -1;
  int min_priority = 10;
  int balance = 0;

  for (i = p; i <= q; i++) {
    if (tokens[i].type == '(') {
      balance++;
      continue;
    }
    if (tokens[i].type == ')') {
      balance--;
      continue;
    }
    if (balance != 0) continue;  /* Inside parentheses */

    if (tokens[i].type == '+' || tokens[i].type == '-' ||
        tokens[i].type == '*' || tokens[i].type == '/' ||
        tokens[i].type == TK_EQ || tokens[i].type == TK_NEQ ||
        tokens[i].type == TK_AND || tokens[i].type == TK_OR) {
      if (tokens[i].priority <= min_priority) {
        min_priority = tokens[i].priority;
        pos = i;
      }
    }
  }

  return pos;
}

/* Evaluate expression recursively */
static uint32_t eval(int p, int q, bool *success) {
  if (p > q) {
    *success = false;
    return 0;
  }

  if (p == q) {
    /* Single token */
    switch (tokens[p].type) {
      case TK_NUM:
        *success = true;
        return atoi(tokens[p].str);
      case TK_HEX:
        *success = true;
        return (uint32_t)strtoul(tokens[p].str, NULL, 16);
      case TK_REG: {
        int i;
        const char *reg_name = tokens[p].str + 1;  /* Skip '$' */
        for (i = 0; i < 8; i++) {
          if (strcmp(reg_name, regsl[i]) == 0) {
            *success = true;
            return cpu.gpr[i]._32;
          }
        }
        if (strcmp(reg_name, "eip") == 0) {
          *success = true;
          return cpu.eip;
        }
        *success = false;
        return 0;
      }
      default:
        *success = false;
        return 0;
    }
  }

  if (check_parentheses(p, q) == true) {
    if (tokens[p].type == '(' && tokens[q].type == ')') {
      return eval(p + 1, q - 1, success);
    }
  } else {
    *success = false;
    return 0;
  }

  /* Find dominant operator */
  int op = find_dominant_operator(p, q);

  if (op == -1) {
    /* No operator found, must be a unary operator */
    if (tokens[p].type == TK_NEG) {
      uint32_t val = eval(p + 1, q, success);
      if (!*success) return 0;
      *success = true;
      return -val;
    }
    if (tokens[p].type == TK_DEREF) {
      uint32_t addr = eval(p + 1, q, success);
      if (!*success) return 0;
      *success = true;
      return vaddr_read(addr, 4);
    }
    if (tokens[p].type == '!') {
      uint32_t val = eval(p + 1, q, success);
      if (!*success) return 0;
      *success = true;
      return !val;
    }
    *success = false;
    return 0;
  }

  uint32_t val1 = eval(p, op - 1, success);
  if (!*success) return 0;
  uint32_t val2 = eval(op + 1, q, success);
  if (!*success) return 0;

  switch (tokens[op].type) {
    case '+': *success = true; return val1 + val2;
    case '-': *success = true; return val1 - val2;
    case '*': *success = true; return val1 * val2;
    case '/':
      if (val2 == 0) {
        *success = false;
        return 0;
      }
      *success = true;
      return val1 / val2;
    case TK_EQ: *success = true; return val1 == val2;
    case TK_NEQ: *success = true; return val1 != val2;
    case TK_AND: *success = true; return val1 && val2;
    case TK_OR: *success = true; return val1 || val2;
    default:
      *success = false;
      return 0;
  }
}

uint32_t expr(char *e, bool *success) {
  if (!make_token(e)) {
    *success = false;
    return 0;
  }

  /* Evaluate the expression */
  return eval(0, nr_token - 1, success);
}
