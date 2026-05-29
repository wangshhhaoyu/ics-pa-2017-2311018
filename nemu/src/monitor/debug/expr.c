#include "nemu.h"

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <sys/types.h>
#include <regex.h>
#include <stdlib.h>

enum {
  TK_NOTYPE = 256, 
  TK_NOT,  // !
  TK_EQ,
  TK_NEQ,  // !=
  TK_LE,   // <=
  TK_GE,   // >=
  TK_AND,  // &
  TK_OR,   // |
  TK_NUM,  // Number
  TK_HEX,  // Hex number
  TK_REG,  // Register
  TK_NEG,   // 单目负号
  TK_POS,   // 单目正号
  TK_DEREF  // 解引用 *
  /* TODO: Add more token types */

};

static struct rule {
  char *regex;
  int token_type;
} rules[] = {

  /* TODO: Add more rules.
   * Pay attention to the precedence level of different rules.
   */

  {" +", TK_NOTYPE},    // spaces

  {"\\*", '*'},           // multiply
  {"/", '/'},             // divide
  {"%", '%'},             // modulo

  {"\\+", '+'},         // plus
  {"-", '-'},             // minus

  {"<=", TK_LE},        // less or equal
  {">=", TK_GE},        // greater or equal
  {"==", TK_EQ},         // equal
  {"!=", TK_NEQ},         // not equal

  {"!", TK_NOT},         // not
  {"&", TK_AND},          // bitwise and
  {"\\|", TK_OR},         // bitwise or

  {"\\(", '('},     // left parenthesis
  {"\\)", ')'},     // right parenthesis
  {"0[xX][0-9a-fA-F]+", TK_HEX},  // hex number
  {"[0-9]+", TK_NUM},     // decimal number
  {"\\$[a-zA-Z]+", TK_REG},      // register
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

        /* TODO: Now a new token is recognized with rules[i]. Add codes
         * to record the token in the array `tokens'. For certain types
         * of tokens, some extra actions should be performed.
         */

        switch (rules[i].token_type) {
          case TK_NOTYPE:
            break;

          case TK_NUM:
          case TK_HEX:
          case TK_REG:
            tokens[nr_token].type = rules[i].token_type;
            if (substr_len >= sizeof(tokens[nr_token].str))
            {
              printf("Token too long!\n");
              return false;
            }
            strncpy(tokens[nr_token].str, substr_start, substr_len);
            tokens[nr_token].str[substr_len] = '\0';
            nr_token++;
            break;

          case TK_NOT:
          case TK_LE:
          case TK_GE:
          case TK_EQ:
          case TK_NEQ:
          case '+':
          case '-':
          case '*':
          case '/':
          case '%':
          case TK_AND:
          case TK_OR:
          case '(':
          case ')':
            tokens[nr_token].type = rules[i].token_type;
            strncpy(tokens[nr_token].str, substr_start, substr_len);
            tokens[nr_token].str[substr_len] = '\0';
            nr_token++;
            break;
          default:
            printf("unknown token type: %d\n", rules[i].token_type);
            return false;
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

static bool is_prev_unary_context(int prev_type)
{
  return prev_type == '(' ||
         prev_type == '+' ||
         prev_type == '-' ||
         prev_type == '*' ||
         prev_type == '/' ||
         prev_type == '%' ||
         prev_type == TK_EQ ||
         prev_type == TK_NEQ ||
         prev_type == TK_LE ||
         prev_type == TK_GE ||
         prev_type == TK_AND ||
         prev_type == TK_OR ||
         prev_type == TK_NOT ||
         prev_type == TK_NEG ||
         prev_type == TK_POS ||
         prev_type == TK_DEREF;
}

static void convert_unary_ops()
{
  for (int i = 0; i < nr_token; i++)
  {
    if (tokens[i].type == '-')
    {
      if (i == 0 || is_prev_unary_context(tokens[i - 1].type))
      {
        tokens[i].type = TK_NEG;
      }
    }
    else if (tokens[i].type == '+')
    {
      if (i == 0 || is_prev_unary_context(tokens[i - 1].type))
      {
        tokens[i].type = TK_POS;
      }
    }
    else if (tokens[i].type == '*')
    {
      if (i == 0 || is_prev_unary_context(tokens[i - 1].type))
      {
        tokens[i].type = TK_DEREF;
      }
    }
    else if (tokens[i].type == '!')
    {
      if (i == 0 || is_prev_unary_context(tokens[i - 1].type))
      {
        tokens[i].type = TK_NOT;
      }
    }
  }
}

static uint32_t eval(int p, int q, bool *success)
{
  if (p > q)
  {
    *success = false;
    return 0;
  }

  if (p == q)
  {
    if (tokens[p].type == TK_NUM)
    {
      return strtoul(tokens[p].str, NULL, 10);
    }
    else if (tokens[p].type == TK_HEX)
    {
      return strtoul(tokens[p].str, NULL, 16);
    }
    else if (tokens[p].type == TK_REG)
    {
      if (strcmp(tokens[p].str, "$eax") == 0)
        return cpu.eax;
      if (strcmp(tokens[p].str, "$ecx") == 0)
        return cpu.ecx;
      if (strcmp(tokens[p].str, "$edx") == 0)
        return cpu.edx;
      if (strcmp(tokens[p].str, "$ebx") == 0)
        return cpu.ebx;
      if (strcmp(tokens[p].str, "$esp") == 0)
        return cpu.esp;
      if (strcmp(tokens[p].str, "$ebp") == 0)
        return cpu.ebp;
      if (strcmp(tokens[p].str, "$esi") == 0)
        return cpu.esi;
      if (strcmp(tokens[p].str, "$edi") == 0)
        return cpu.edi;
      if (strcmp(tokens[p].str, "$eip") == 0)
        return cpu.eip;
      printf("unknown register: %s\n", tokens[p].str);
      *success = false;
      return 0;
    }
    printf("unexpected token type: %d\n", tokens[p].type);
    *success = false;
    return 0;
  }

  if (tokens[p].type == '(' && tokens[q].type == ')')
  {
    int level = 0;
    bool whole_wrapped = true;
    for (int i = p; i <= q; i++)
    {
      if (tokens[i].type == '(')
        level++;
      else if (tokens[i].type == ')')
        level--;
      if (level == 0 && i < q)
      {
        whole_wrapped = false;
        break;
      }
    }
    if (whole_wrapped)
    {
      return eval(p + 1, q - 1, success);
    }
  }

  int op_pos = -1;
  int paren_level = 0;
  int min_prec = 10;

#define PREC_ADD_SUB 1
#define PREC_MUL_DIV_MOD 2
#define PREC_EQ 3
#define PREC_AND 4
#define PREC_OR 5
#define PREC_UNARY 6

  for (int i = q; i >= p; i--)
  {
    int t = tokens[i].type;

    if (t == ')')
    {
      paren_level++;
    }
    else if (t == '(')
    {
      paren_level--;
    }
    else if (paren_level == 0)
    {
      int curr_prec = 0;

      if (t == '+' || t == '-')
      {
        curr_prec = PREC_ADD_SUB;
      }
      else if (t == '*' || t == '/' || t == '%')
      {
        curr_prec = PREC_MUL_DIV_MOD;
      }
      else if (t == TK_EQ || t == TK_NEQ || t == TK_LE || t == TK_GE)
      {
        curr_prec = PREC_EQ;
      }
      else if (t == TK_AND)
      {
        curr_prec = PREC_AND;
      }
      else if (t == TK_OR)
      {
        curr_prec = PREC_OR;
      }

      if (curr_prec != 0 && curr_prec <= min_prec)
      {
        min_prec = curr_prec;
        op_pos = i;
      }
    }
  }

  if (op_pos == -1)
  {
    for (int i = p; i <= q; i++)
    {
      int t = tokens[i].type;
      if (t == TK_NEG || t == TK_POS || t == TK_DEREF || t == TK_NOT)
      {
        uint32_t val = eval(i + 1, q, success);
        if (!(*success))
          return 0;
        switch (t)
        {
        case TK_NEG:
          return -val;
        case TK_POS:
          return val;
        case TK_NOT:
          return (val == 0) ? 1 : 0;
        case TK_DEREF:
          return paddr_read(val, 4);
        }
      }
    }

    printf("no dominant operator found\n");
    *success = false;
    return 0;
  }

  uint32_t left = eval(p, op_pos - 1, success);
  if (!(*success))
    return 0;
  uint32_t right = eval(op_pos + 1, q, success);
  if (!(*success))
    return 0;

  switch (tokens[op_pos].type)
  {
  case '+':
    return left + right;
  case '-':
    return left - right;
  case '*':
    return left * right;
  case '/':
    if (right == 0)
    {
      printf("division by zero\n");
      *success = false;
      return 0;
    }
    return left / right;
  case '%':
    if (right == 0)
    {
      printf("modulo by zero\n");
      *success = false;
      return 0;
    }
    return left % right;
  case TK_EQ:
    return left == right;
  case TK_NEQ:
    return left != right;
  case TK_LE:
    return left <= right;
  case TK_GE:
    return left >= right;
  case TK_AND:
    return left & right;
  case TK_OR:
    return left | right;
  default:
    printf("unknown operator: %d\n", tokens[op_pos].type);
    *success = false;
    return 0;
  }
}

uint32_t expr(char *e, bool *success) {
  if (!make_token(e)) {
    *success = false;
    return 0;
  }

  /* TODO: Insert codes to evaluate the expression. */
  convert_unary_ops();
  *success = true;
  return eval(0, nr_token - 1, success);
}
