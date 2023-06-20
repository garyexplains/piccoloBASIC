/*
 * Copyright (c) 2006, Adam Dunkels
 * Copyright (c) 2023, Gary Sims
 * All rights reserved.
 *
 * Bug fixes 2021, Alessio Villa (see
 * https://github.com/adamdunkels/ubasic/issues/4)
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the author nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <math.h>
#include <limits.h>

#include "pico/stdlib.h"

#define DEBUG 1

#if DEBUG
#define DEBUG_PRINTF(...) printf(__VA_ARGS__)
#else
#define DEBUG_PRINTF(...)
#endif

#define DDEBUG 0

#if DDEBUG
#define DDEBUG_PRINTF(...) printf(__VA_ARGS__)
#else
#define DDEBUG_PRINTF(...)
#endif

#include "tokenizer.h"
#include "ubasic.h"
#include "piccoloBASIC.h"

static char const *program_ptr;
#define MAX_STRINGLEN 128
static char string[MAX_STRINGLEN];

#define MAX_GOSUB_STACK_DEPTH 10
static int gosub_stack[MAX_GOSUB_STACK_DEPTH];
static int gosub_stack_ptr;

#define MAX_INT_STACK_DEPTH 256
static int int_stack[MAX_INT_STACK_DEPTH];
static int int_stack_ptr;

struct for_state {
  int line_after_for;
  int for_variable;
  int to;
};
#define MAX_FOR_STACK_DEPTH 4
static struct for_state for_stack[MAX_FOR_STACK_DEPTH];
static int for_stack_ptr;

static int gline_number;

static unsigned long RANDOM_NUM_SEED_x=123456789;

#define MAX_VARNUM 26

struct line_index {
  int line_number;
  char label[MAX_LABELLEN];
  char const *program_text_position;
  struct line_index *next;
};
struct line_index *line_index_head = NULL;
struct line_index *line_index_current = NULL;

static VARIABLE_TYPE variables[MAX_VARNUM];
static VARFLOAT_TYPE float_variables[MAX_VARNUM];
static VARSTRING_TYPE string_variables[MAX_VARNUM];

static int ended;

static VARIABLE_TYPE expr(void);
static VARFLOAT_TYPE exprf(void);
static VARSTRING_TYPE exprs(void);
static void line_statement(void);
static void statement(void);
static void index_free(void);
static void index_add_label(int linenum, char *label);
static VARIABLE_TYPE builtin(int token, int p);
static VARFLOAT_TYPE builtinf(int token, VARFLOAT_TYPE p);
static VARSTRING_TYPE builtinstr(int token, VARSTRING_TYPE p);
static VARSTRING_TYPE sprintfloat(VARFLOAT_TYPE f);
static void printfloat(VARFLOAT_TYPE f);

peek_func peek_function = NULL;
poke_func poke_function = NULL;

void poke(VARIABLE_TYPE arg, VARIABLE_TYPE value) {
    volatile VARIABLE_TYPE *addr = (VARIABLE_TYPE *) arg;
    *addr = value;
}

/*---------------------------------------------------------------------------*/
void ubasic_init(const char *program) {
  program_ptr = program;
  for_stack_ptr = gosub_stack_ptr = 0;
  index_free();
  peek_function = NULL;
  poke_function = poke;
  tokenizer_init(program);
  gline_number = 1;
  ended = 0;
  for(int i=0;i<MAX_VARNUM;i++) {
    variables[i] = 0;
    float_variables[i] = 0.0;
    string_variables[i] = NULL;
  }
}
char ubasic_exit_buffer[64];
char *ubasic_exit_static_itoa(int e) {
  sprintf(ubasic_exit_buffer, "%d", e);
  return ubasic_exit_buffer;
}
void ubasic_exit(int errline, char *errmsg, char *errp) {
  int lessoften = 0;
  // Never actually return/exit
  while (true) {
    check_if_should_enter_CMD_mode();
    sleep_ms(500);
    if( (lessoften++ % 10) == 0)
      printf("Error on line %d - %s (%s)\n", errline, errmsg, errp);
  }
}
/*---------------------------------------------------------------------------*/
void ubasic_init_peek_poke(const char *program, peek_func peek,
                           poke_func poke) {
  program_ptr = program;
  for_stack_ptr = gosub_stack_ptr = 0;
  index_free();
  peek_function = peek;
  poke_function = poke;
  tokenizer_init(program);
  ended = 0;
}
/*---------------------------------------------------------------------------*/
static void accept(int token) {
  if (token != tokenizer_token()) {
    DEBUG_PRINTF("Token not what was expected (expected %d, got %d)\n", token,
                 tokenizer_token());
    tokenizer_error_print(gline_number - 1, "Unexpected token");
    ubasic_exit(gline_number - 1, "Unexpected token", ubasic_exit_static_itoa(token));
  }
  DEBUG_PRINTF("Expected %d, got it\n", token);
  tokenizer_next();
}
/*---------------------------------------------------------------------------*/
static int varfactor(void) {
  VARIABLE_TYPE r;
  DEBUG_PRINTF("varfactor: obtaining %ld from variable %d\n",
               variables[tokenizer_variable_num()], tokenizer_variable_num());
  r = ubasic_get_variable(tokenizer_variable_num());
  accept(TOKENIZER_VARIABLE);
  return r;
}
/*---------------------------------------------------------------------------*/
static VARFLOAT_TYPE varfloatfactor(void) {
  VARFLOAT_TYPE f;
  DEBUG_PRINTF("varfloatfactor: obtaining %f from variable %d\n",
               float_variables[tokenizer_variable_num()],
               tokenizer_variable_num());
  f = ubasic_get_float_variable(tokenizer_variable_num());
  accept(TOKENIZER_VARFLOAT);
  return f;
}
/*---------------------------------------------------------------------------*/
static VARSTRING_TYPE varstrfactor(void) {
  VARSTRING_TYPE s;
  DEBUG_PRINTF("varstrfactor: obtaining %s from variable %d\n",
               string_variables[tokenizer_variable_num()],
               tokenizer_variable_num());
  s = ubasic_get_string_variable(tokenizer_variable_num());
  accept(TOKENIZER_VARSTRING);
  return s;
}
/*---------------------------------------------------------------------------*/
static int factor(void) {
  VARIABLE_TYPE r;
  VARIABLE_TYPE p;
  int builtin_token;

  DEBUG_PRINTF("factor: token %d\n", tokenizer_token());
  switch (tokenizer_token()) {
  case TOKENIZER_NUMBER:
    r = tokenizer_num();
    DEBUG_PRINTF("factor: number %ld\n", r);
    accept(TOKENIZER_NUMBER);
    break;
  case TOKENIZER_ZERO:
  case TOKENIZER_NOT:
  case TOKENIZER_RANDINT:
  case TOKENIZER_TIME:
    builtin_token = tokenizer_token();
    accept(builtin_token);
    accept(TOKENIZER_LEFTPAREN);
    if (tokenizer_token() == TOKENIZER_RIGHTPAREN) {
      p = 0;
      accept(TOKENIZER_RIGHTPAREN);
    } else {
      p = expr();
      accept(TOKENIZER_RIGHTPAREN);
    }
    r = builtin(builtin_token, p);
    DEBUG_PRINTF("builtin %d(%ld)=%ld\n", builtin_token, p, r);
    break;
  case TOKENIZER_LEFTPAREN:
    accept(TOKENIZER_LEFTPAREN);
    r = expr();
    accept(TOKENIZER_RIGHTPAREN);
    break;
  case TOKENIZER_VARFLOAT:
    r = (VARIABLE_TYPE)varfloatfactor();
    break;
  default:
    r = varfactor();
    break;
  }
  return r;
}
/*---------------------------------------------------------------------------*/
static VARFLOAT_TYPE factorf(void) {
  VARFLOAT_TYPE f;
  VARFLOAT_TYPE p;
  int builtin_token;

  DEBUG_PRINTF("factorf: token %d\n", tokenizer_token());
  switch (tokenizer_token()) {
  case TOKENIZER_NUMBER:
    f = (VARFLOAT_TYPE)tokenizer_num();
    DEBUG_PRINTF("factorf: number %f\n", f);
    accept(TOKENIZER_NUMBER);
    break;
  case TOKENIZER_NUMFLOAT:
    f = tokenizer_numfloat();
    DEBUG_PRINTF("factorf: float number %f\n", f);
    accept(TOKENIZER_NUMFLOAT);
    break;
  case TOKENIZER_SQR:
  case TOKENIZER_RND:
  case TOKENIZER_ABS:
  case TOKENIZER_ATN:
  case TOKENIZER_COS:
  case TOKENIZER_EXP:
  case TOKENIZER_LOG:
  case TOKENIZER_SIN:
  case TOKENIZER_TAN:
    builtin_token = tokenizer_token();
    accept(builtin_token);
    accept(TOKENIZER_LEFTPAREN);
    if (tokenizer_token() == TOKENIZER_RIGHTPAREN) {
      p = 0.0;
      accept(TOKENIZER_RIGHTPAREN);
    } else {
      p = expr();
      accept(TOKENIZER_RIGHTPAREN);
    }
    f = builtinf(builtin_token, p);
    DEBUG_PRINTF("builtinf %d(%f)=%f\n", builtin_token, p, f);
    break;
  case TOKENIZER_LEFTPAREN:
    accept(TOKENIZER_LEFTPAREN);
    f = exprf();
    accept(TOKENIZER_RIGHTPAREN);
    break;
  case TOKENIZER_VARIABLE:
    f = (VARFLOAT_TYPE)varfactor();
    break;
  default:
    f = varfloatfactor();
    break;
  }
  return f;
}
/*---------------------------------------------------------------------------*/
static int term(void) {
  VARIABLE_TYPE f1, f2;
  int op;

  f1 = factor();
  op = tokenizer_token();
  DEBUG_PRINTF("term: token %d\n", op);
  while (op == TOKENIZER_ASTR || op == TOKENIZER_SLASH || op == TOKENIZER_MOD) {
    tokenizer_next();
    f2 = factor();
    DEBUG_PRINTF("term: %ld %d %ld\n", f1, op, f2);
    switch (op) {
    case TOKENIZER_ASTR:
      f1 = f1 * f2;
      break;
    case TOKENIZER_SLASH:
      f1 = f1 / f2;
      break;
    case TOKENIZER_MOD:
      f1 = f1 % f2;
      break;
    }
    op = tokenizer_token();
  }
  DEBUG_PRINTF("term: %ld\n", f1);
  return f1;
}
/*---------------------------------------------------------------------------*/
static VARIABLE_TYPE expr(void) {
  VARIABLE_TYPE t1, t2;
  int op;

  t1 = term();
  op = tokenizer_token();
  DEBUG_PRINTF("expr: token %d\n", op);
  while (op == TOKENIZER_PLUS || op == TOKENIZER_MINUS || op == TOKENIZER_AND ||
         op == TOKENIZER_OR) {
    tokenizer_next();
    t2 = term();
    DEBUG_PRINTF("expr: %ld %d %ld\n", t1, op, t2);
    switch (op) {
    case TOKENIZER_PLUS:
      t1 = t1 + t2;
      break;
    case TOKENIZER_MINUS:
      t1 = t1 - t2;
      break;
    case TOKENIZER_AND:
      t1 = t1 & t2;
      break;
    case TOKENIZER_OR:
      t1 = t1 | t2;
      break;
    }
    op = tokenizer_token();
  }
  DEBUG_PRINTF("expr: %ld\n", t1);
  return t1;
}
/*---------------------------------------------------------------------------*/
static VARFLOAT_TYPE termf(void) {
  VARFLOAT_TYPE f1, f2;
  int op;

  f1 = factorf();
  op = tokenizer_token();
  DEBUG_PRINTF("termf: token %d\n", op);
  while (op == TOKENIZER_ASTR || op == TOKENIZER_SLASH) {
    tokenizer_next();
    f2 = factorf();
    DEBUG_PRINTF("termf: %f %d %f\n", f1, op, f2);
    switch (op) {
    case TOKENIZER_ASTR:
      f1 = f1 * f2;
      break;
    case TOKENIZER_SLASH:
      f1 = f1 / f2;
      break;
    }
    op = tokenizer_token();
  }
  DEBUG_PRINTF("termf: %f\n", f1);
  return f1;
}
/*---------------------------------------------------------------------------*/
static VARFLOAT_TYPE exprf(void) {
  VARFLOAT_TYPE t1, t2;
  int op;

  t1 = termf();
  op = tokenizer_token();
  DEBUG_PRINTF("exprf: token %d\n", op);
  while (op == TOKENIZER_PLUS || op == TOKENIZER_MINUS) {
    tokenizer_next();
    t2 = termf();
    DEBUG_PRINTF("exprf: %f %d %f\n", t1, op, t2);
    switch (op) {
    case TOKENIZER_PLUS:
      t1 = t1 + t2;
      break;
    case TOKENIZER_MINUS:
      t1 = t1 - t2;
      break;
    }
    op = tokenizer_token();
  }
  DEBUG_PRINTF("exprf: %f\n", t1);
  return t1;
}
/*---------------------------------------------------------------------------*/
static VARSTRING_TYPE factors(void) {
  VARSTRING_TYPE s;
  VARSTRING_TYPE p;
  int builtin_token;
  char buff[64];

  DEBUG_PRINTF("factors: token %d\n", tokenizer_token());
  switch (tokenizer_token()) {
  case TOKENIZER_NUMBER:
    // Potential overflow bug
    sprintf(buff,"%d",tokenizer_num());
    s = strdup(buff);
    DEBUG_PRINTF("factors: number %s\n", s);
    accept(TOKENIZER_NUMBER);
    break;
  case TOKENIZER_NUMFLOAT:
    // Potential overflow bug
    s = sprintfloat(tokenizer_numfloat());
    // sprintf(buff, "%f", tokenizer_numfloat());
    // s = strdup(buff);
    DEBUG_PRINTF("factors: float number %s\n", s);
    accept(TOKENIZER_NUMFLOAT);
    break;
  case TOKENIZER_LEN:
    builtin_token = tokenizer_token();
    accept(builtin_token);
    accept(TOKENIZER_LEFTPAREN);
    if (tokenizer_token() == TOKENIZER_RIGHTPAREN) {
      p = NULL;
      accept(TOKENIZER_RIGHTPAREN);
    } else {
      p = exprs();
      accept(TOKENIZER_RIGHTPAREN);
    }
    s = builtinstr(builtin_token, p);
    DEBUG_PRINTF("builtins %d(%s)=%s\n", builtin_token, p, s);
    break;
  case TOKENIZER_LEFTPAREN:
    accept(TOKENIZER_LEFTPAREN);
    s = exprs();
    accept(TOKENIZER_RIGHTPAREN);
    break;
  case TOKENIZER_VARIABLE:
    // Potential overflow bug
    sprintf(buff,"%d",varfactor());
    s = strdup(buff);
    break;
  case TOKENIZER_VARSTRING:
    s = strdup(varstrfactor());
    break;
  case TOKENIZER_STRING:
    tokenizer_string(string, sizeof(string));
    s = strdup(string);
    accept(TOKENIZER_STRING);
    break;
  default:
    // TOKENIZER_VARFLOAT
    // Potential overflow bug
    sprintf(buff,"%f",varfloatfactor());
    s = strdup(buff);
    break;
  }
  return s;
}
/*---------------------------------------------------------------------------*/
static VARSTRING_TYPE terms(void) {
  VARSTRING_TYPE s1, s2;
  int op;

  s1 = factors();
  DEBUG_PRINTF("terms: %s\n", s1);
  return s1;
}
/*---------------------------------------------------------------------------*/
static VARSTRING_TYPE exprs(void) {
  VARSTRING_TYPE t1;
  VARSTRING_TYPE t2;
  VARSTRING_TYPE r1;
  VARSTRING_TYPE t1_temp;
  int op;

  r1 = NULL;

  t1 = terms();
  r1 = strdup(t1);
  op = tokenizer_token();
  DEBUG_PRINTF("exprs: token %d\n", op);
  while (op == TOKENIZER_PLUS) {
    tokenizer_next();
    t2 = terms();
    DEBUG_PRINTF("exprs: %s %d %s\n", t1, op, t2);
    switch (op) {
    case TOKENIZER_PLUS:
      t1_temp = t1;
      t1 = malloc(strlen(t1_temp) + strlen(t2) + 2);
      sprintf(t1, "%s%s", t1_temp, t2);
      if(t1_temp!=NULL)
         free(t1_temp);
      break;
    }
    op = tokenizer_token();
  }
  DEBUG_PRINTF("exprs: %s\n", t1);
  return t1;
}
/*---------------------------------------------------------------------------*/
static int relation(void) {
  int r1, r2;
  int op;

  r1 = expr();
  op = tokenizer_token();
  DEBUG_PRINTF("relation: token %d\n", op);
  while (op == TOKENIZER_LT || op == TOKENIZER_GT || op == TOKENIZER_EQ) {
    tokenizer_next();
    r2 = expr();
    DEBUG_PRINTF("relation: %d %d %d\n", r1, op, r2);
    switch (op) {
    case TOKENIZER_LT:
      r1 = r1 < r2;
      break;
    case TOKENIZER_GT:
      r1 = r1 > r2;
      break;
    case TOKENIZER_EQ:
      r1 = r1 == r2;
      break;
    }
    op = tokenizer_token();
  }
  return r1;
}
/*---------------------------------------------------------------------------*/
static void index_free(void) {
  if (line_index_head != NULL) {
    line_index_current = line_index_head;
    do {
      DDEBUG_PRINTF("Freeing index for line %d.\n",
                   line_index_current->line_number);
      line_index_head = line_index_current;
      line_index_current = line_index_current->next;
      free(line_index_head);
    } while (line_index_current != NULL);
    line_index_head = NULL;
  }
}
/*---------------------------------------------------------------------------*/
static char const *index_find(int linenum) {
  struct line_index *lidx;
  lidx = line_index_head;

#if DEBUG
  int step = 0;
#endif

  while (lidx != NULL && lidx->line_number != linenum) {
    lidx = lidx->next;

#if DEBUG
    if (lidx != NULL) {
      DDEBUG_PRINTF("index_find: Step %3d. Found index for line %d: %p.\n", step,
                   lidx->line_number, (void *)lidx->program_text_position);
    }
    step++;
#endif
  }
  if (lidx != NULL && lidx->line_number == linenum) {
    DDEBUG_PRINTF("index_find: Returning index for line %d.\n", linenum);
    return lidx->program_text_position;
  }
  DEBUG_PRINTF("index_find: Returning NULL for %d.\n", linenum);
  return NULL;
}
/*---------------------------------------------------------------------------*/
static char const *index_find_by_pos(const char *pos) {
  struct line_index *lidx;
  lidx = line_index_head;

#if DEBUG
  int step = 0;
#endif

  while (lidx != NULL && lidx->program_text_position != pos) {
    lidx = lidx->next;

#if DEBUG
    if (lidx != NULL) {
      DDEBUG_PRINTF(
          "index_find_by_pos: Step %3d. Found index for %p at line %d: %p.\n",
          step, pos, lidx->line_number, (void *)lidx->program_text_position);
    }
    step++;
#endif
  }
  if (lidx != NULL && lidx->program_text_position == pos) {
    DDEBUG_PRINTF("index_find_by_pos: Returning index for line %d.\n",
                 lidx->line_number);
    return lidx->program_text_position;
  }
  DDEBUG_PRINTF("index_find_by_pos: Returning NULL for %p.\n", pos);
  return NULL;
}

/*---------------------------------------------------------------------------*/
static int linenum_find_by_pos(const char *pos) {
  struct line_index *lidx;
  lidx = line_index_head;

#if DEBUG
  int step = 0;
#endif

  while (lidx != NULL && lidx->program_text_position != pos) {
    lidx = lidx->next;

#if DEBUG
    if (lidx != NULL) {
      DDEBUG_PRINTF(
          "linenum_find_by_pos: Step %3d. Found index for %p at line %d: %p.\n",
          step, pos, lidx->line_number, (void *)lidx->program_text_position);
    }
    step++;
#endif
  }
  if (lidx != NULL && lidx->program_text_position == pos) {
    DDEBUG_PRINTF("linenum_find_by_pos: Returning line %d for index %p.\n",
                 lidx->line_number, pos);
    return lidx->line_number;
  }
  DDEBUG_PRINTF("linenum_find_by_pos: Returning NULL for %p.\n", pos);
  return -1;
}
/*---------------------------------------------------------------------------*/
static char const *index_find_by_label(char *label) {
  struct line_index *lidx;
  lidx = line_index_head;

#if DEBUG
  int step = 0;
#endif

  while ((lidx != NULL) && (strcmp(lidx->label, label) != 0)) {
    lidx = lidx->next;

#if DEBUG
    if (lidx != NULL) {
      DDEBUG_PRINTF(
          "index_find_by_label: Step %3d. Found index for label %s: %p.\n",
          step, lidx->label, (void *)lidx->program_text_position);
    }
    step++;
#endif
  }
  if ((lidx != NULL) && (strcmp(lidx->label, label) == 0)) {
    DDEBUG_PRINTF("index_find_by_label: Returning index for label %s.\n", label);
    return lidx->program_text_position;
  }
  DDEBUG_PRINTF("index_find_by_label: Returning NULL for %s.\n", label);
  return NULL;
}
/*---------------------------------------------------------------------------*/
static void index_add_label(int linenum, char *label) {
  struct line_index *lidx;
  lidx = line_index_head;

#if DEBUG
  int step = 0;
#endif

  while (lidx != NULL && lidx->line_number != linenum) {
    lidx = lidx->next;

#if DEBUG
    if (lidx != NULL) {
      DDEBUG_PRINTF("index_add_label: Step %3d. Found index for line %d: %p.\n",
                   step, lidx->line_number,
                   (void *)lidx->program_text_position);
    }
    step++;
#endif
  }
  if (lidx != NULL && lidx->line_number == linenum) {
    DDEBUG_PRINTF("index_add_label: Setting label to %s for line %d.\n", label,
                 linenum);
    strcpy(lidx->label, label);
    return;
  }
  DDEBUG_PRINTF("index_add_label: Label not set for line %d.\n", linenum);
  return;
}

/*---------------------------------------------------------------------------*/
static void index_add(int linenum, char const *sourcepos) {
  if (line_index_head != NULL && index_find(linenum)) {
    return;
  }

  struct line_index *new_lidx;

  new_lidx = malloc(sizeof(struct line_index));
  new_lidx->line_number = linenum;
  new_lidx->program_text_position = sourcepos;
  new_lidx->next = NULL;

  if (line_index_head != NULL) {
    line_index_current->next = new_lidx;
    line_index_current = line_index_current->next;
  } else {
    line_index_current = new_lidx;
    line_index_head = line_index_current;
  }
  DDEBUG_PRINTF("index_add: Adding index for line %d: %p.\n", linenum,
               (void *)sourcepos);
}
/*---------------------------------------------------------------------------*/
static void jump_linenum_slow(int linenum) {
  int lc = 1;
  int last_token = TOKENIZER_ERROR;
  int err_lc = gline_number - 1;

  DEBUG_PRINTF("jump_linenum_slow: start\n");
  tokenizer_init(program_ptr);
  do {
    last_token = tokenizer_token();
    if (last_token == TOKENIZER_CR)
      lc++;

    tokenizer_next();
    if (lc == linenum) {
      gline_number = lc;
      DEBUG_PRINTF("## jump_linenum_slow: returning at line %d\n", lc);
      return;
    }
  } while (tokenizer_token() != TOKENIZER_ENDOFINPUT);
  printf("Error: On line %d, line %d not found\n", err_lc, linenum);
}
/*---------------------------------------------------------------------------*/
static void jump_linenum(int linenum) {
  char const *pos = index_find(linenum);
  DEBUG_PRINTF("jump_linenum: Trying to go to line %d.\n", linenum);
  if (pos != NULL) {
    DEBUG_PRINTF("jump_linenum: Going to line %d.\n", linenum);
    tokenizer_goto(pos);
  } else {
    /* We'll try to find a yet-unindexed line to jump to. */
    DEBUG_PRINTF("jump_linenum: Calling jump_linenum_slow for %d.\n", linenum);
    jump_linenum_slow(linenum);
  }
}
/*---------------------------------------------------------------------------*/
static void jump_label_slow(char *label) {
  char l[MAX_LABELLEN];
  int last_token = TOKENIZER_ERROR;
  int lc = 1;
  int err_lc = gline_number - 1;

  DEBUG_PRINTF("jump_label_slow: start\n");
  tokenizer_init(program_ptr);
  do {
    last_token = tokenizer_token();
    if (last_token == TOKENIZER_CR)
      lc++;
    tokenizer_next();
    if ((tokenizer_token() == TOKENIZER_LABEL) &&
        ((last_token != TOKENIZER_GOSUB) && (last_token != TOKENIZER_GOTO))) {
      tokenizer_label(l, MAX_LABELLEN);
      DEBUG_PRINTF(
          "== jump_label_slow: found a label %s with last_token of %d\n", l,
          last_token);
      if (strcmp(label, l) == 0) {
        gline_number = lc;
        DEBUG_PRINTF("## jump_label_slow: returning at line %d\n", lc);
        return;
      }
    }
  } while (tokenizer_token() != TOKENIZER_ENDOFINPUT);
  printf("Error: On line %d, label %s not found\n", err_lc, label);
}
/*---------------------------------------------------------------------------*/
static void jump_label(char *label) {
  char const *pos = index_find_by_label(label);
  DEBUG_PRINTF("jump_label: Trying to go to label %s.\n", label);
  if (pos != NULL) {
    DEBUG_PRINTF("jump_label: Going to label %s.\n", label);
    tokenizer_goto(pos);
  } else {
    /* We'll try to find a yet-unindexed line to jump to. */
    DEBUG_PRINTF("jump_label: Calling jump_label_slow for %s.\n", label);
    jump_label_slow(label);
  }
}
/*---------------------------------------------------------------------------*/
static VARIABLE_TYPE builtin(int token, int p) {
  time_t seconds;

  if (token <= TOKENIZER_BUILTINS__START || token > TOKENIZER_BUILTINS__END) {
    printf("Error: Invalid builtin function %d (%d)\n", token, p);
    ubasic_exit(gline_number - 1, "Invalid builtin function", ubasic_exit_static_itoa(token));
  }

  switch (token) {
  case TOKENIZER_ZERO:
    return 0;
    break;
  case TOKENIZER_NOT:
    if (p == 0)
      return 1;
    else
      return 0;
    break;
  case TOKENIZER_RANDINT:
    return abs((VARIABLE_TYPE)(RANDOM_NUM_SEED_x =
                                   69069 * RANDOM_NUM_SEED_x + 362437));
    break;
  case TOKENIZER_TIME:    
    seconds = time(NULL);
    return (VARIABLE_TYPE)seconds;
    break;
  default:
    break;
  }

  return 0;
}
/*---------------------------------------------------------------------------*/
static VARFLOAT_TYPE builtinf(int token, VARFLOAT_TYPE p) {
  int r;

  if (token <= TOKENIZER_BUILTINSF__START || token > TOKENIZER_BUILTINSF__END) {
    printf("Error: Invalid builtinf function %d (%f)\n", token, p);
    ubasic_exit(gline_number - 1, "Invalid builtinf function", ubasic_exit_static_itoa(token));
  }

  switch (token) {
  case TOKENIZER_SQR:
    return sqrt(p);
    break;
  case TOKENIZER_RND:
    r = abs((RANDOM_NUM_SEED_x = 69069 * RANDOM_NUM_SEED_x + 362437));
    return (double)(r) / (double)INT_MAX;
    break;
  case TOKENIZER_ABS:
    return fabs(p);
    break;
  case TOKENIZER_ATN:
    return atan(p);
    break;
  case TOKENIZER_COS:
    return cos(p);
    break;
  case TOKENIZER_EXP:
    return exp(p);
    break;
  case TOKENIZER_LOG:
    return log(p);
    break;
  case TOKENIZER_SIN:
    return sin(p);
    break;
  case TOKENIZER_TAN:
    return tan(p);
    break;
  default:
    break;
  }

  return 0.0;
}
/*---------------------------------------------------------------------------*/
static VARSTRING_TYPE builtinstr(int token, VARSTRING_TYPE p) {
  int r;

  // if (token <= TOKENIZER_BUILTINSF__START || token > TOKENIZER_BUILTINSF__END) {
  //   printf("Error: Invalid builtinf function %d (%f)\n", token, p);
  //   ubasic_exit(-1);
  // }

  switch (token) {
  case TOKENIZER_LEN:
    return NULL;
    break;
  default:
    break;
  }

  return NULL;
}
/*---------------------------------------------------------------------------*/
static void goto_statement(void) {
  char l[MAX_LABELLEN];
  accept(TOKENIZER_GOTO);
  DEBUG_PRINTF("Enter goto_statement\n");
  tokenizer_label(l, MAX_LABELLEN);
  accept(TOKENIZER_LABEL);
  if (tokenizer_token() == TOKENIZER_CR) {
    accept(TOKENIZER_CR);
  }
  jump_label(l);
}
/*---------------------------------------------------------------------------*/
// Potential overflow bug
static void printfloat(VARFLOAT_TYPE f) {

  char buff[48];
  int len;
  len = snprintf(buff, sizeof buff, "%f", f);
  DEBUG_PRINTF("printfloat: %s\n", buff);
  char *p = buff + len - 1;
  while (*p == '0') {
    *p-- = 0;
  }
  if (*p == '.') {
    *(p + 1) = '0';
    *(p + 2) = 0;
  }
  printf("%s", buff);
}
/*---------------------------------------------------------------------------*/
// Potential overflow bug
static VARSTRING_TYPE sprintfloat(VARFLOAT_TYPE f) {

  char buff[48];
  int len;
  len = snprintf(buff, sizeof buff, "%f", f);
  DEBUG_PRINTF("sprintfloat: %s\n", buff);
  char *p = buff + len - 1;
  while (*p == '0') {
    *p-- = 0;
  }
  if (*p == '.') {
    *(p + 1) = '0';
    *(p + 2) = 0;
  }
  return strdup(buff);
}
/*---------------------------------------------------------------------------*/
static void print_statement(void) {
  accept(TOKENIZER_PRINT);
  do {
    DEBUG_PRINTF("Print loop\n");
    if (tokenizer_token() == TOKENIZER_STRING) {
      tokenizer_string(string, sizeof(string));
      printf("%s", string);
      tokenizer_next();
    } else if (tokenizer_token() == TOKENIZER_VARSTRING) {
      printf("%s", exprs());
    } else if (tokenizer_token() == TOKENIZER_COMMA) {
      printf(" ");
      tokenizer_next();
    } else if (tokenizer_token() == TOKENIZER_SEMICOLON) {
      tokenizer_next();
    } else if (tokenizer_token() == TOKENIZER_VARIABLE ||
               tokenizer_token() == TOKENIZER_NUMBER ||
               (tokenizer_token() > TOKENIZER_BUILTINS__START && tokenizer_token() < TOKENIZER_BUILTINS__END)) {
      printf("%d", expr());
    } else if (tokenizer_token() == TOKENIZER_VARFLOAT ||
               tokenizer_token() == TOKENIZER_NUMFLOAT ||
               (tokenizer_token() > TOKENIZER_BUILTINSF__START && tokenizer_token() < TOKENIZER_BUILTINSF__END)) {
      printfloat(exprf());
    } else {
      break;
    }
  } while (tokenizer_token() != TOKENIZER_CR &&
           tokenizer_token() != TOKENIZER_ENDOFINPUT);
  printf("\n");
  DEBUG_PRINTF("End of print\n");
  tokenizer_next();
}
/*---------------------------------------------------------------------------*/
static void os_statement(void) {
  accept(TOKENIZER_OS);
  do {
    DEBUG_PRINTF("OS loop\n");
    if (tokenizer_token() == TOKENIZER_STRING) {
      tokenizer_string(string, sizeof(string));
      system(string);
      tokenizer_next();
    } else if (tokenizer_token() == TOKENIZER_VARSTRING) {
      system(exprs());
    } else {
      break;
    }
  } while (tokenizer_token() != TOKENIZER_CR &&
           tokenizer_token() != TOKENIZER_ENDOFINPUT);
  printf("\n");
  DEBUG_PRINTF("End of OS statement\n");
  tokenizer_next();
}
/*---------------------------------------------------------------------------*/
static void if_statement(void) {
  int r;
  DEBUG_PRINTF("if_statement start\n");
  accept(TOKENIZER_IF);

  r = relation();
  DEBUG_PRINTF("if_statement: relation %d\n", r);
  accept(TOKENIZER_THEN);
  if (r) {
    statement();
    if (tokenizer_token() == TOKENIZER_ELSE) {
      accept(TOKENIZER_ELSE);
      while (tokenizer_token() != TOKENIZER_CR &&
             tokenizer_token() != TOKENIZER_ENDOFINPUT) {
        tokenizer_next();
      }
    }
    if (tokenizer_token() == TOKENIZER_CR) {
      accept(TOKENIZER_CR);
    }
    DEBUG_PRINTF("if_statement end of true\n");
  } else {
    do {
      tokenizer_next();
    } while (tokenizer_token() != TOKENIZER_ELSE &&
             tokenizer_token() != TOKENIZER_CR &&
             tokenizer_token() != TOKENIZER_ENDOFINPUT);
    if (tokenizer_token() == TOKENIZER_ELSE) {
      tokenizer_next();
      statement();
    } else if (tokenizer_token() == TOKENIZER_CR) {
      tokenizer_next();
    }
    DEBUG_PRINTF("if_statement end of false\n");
  }
  DEBUG_PRINTF("if_statement end\n");
}
/*---------------------------------------------------------------------------*/
static void let_statement(void) {
  int var;

  if (tokenizer_token() == TOKENIZER_VARIABLE) {
    var = tokenizer_variable_num();

    accept(TOKENIZER_VARIABLE);
    accept(TOKENIZER_EQ);
    ubasic_set_variable(var, expr());
    DEBUG_PRINTF("let_statement: assign %d to %d\n", variables[var], var);
    if (tokenizer_token() == TOKENIZER_CR)
      tokenizer_next();
  } else if (tokenizer_token() == TOKENIZER_VARFLOAT) {
    var = tokenizer_variable_num();

    accept(TOKENIZER_VARFLOAT);
    accept(TOKENIZER_EQ);
    ubasic_set_float_variable(var, exprf());
    DEBUG_PRINTF("let_statement: assign %f to %d\n", float_variables[var], var);
    if (tokenizer_token() == TOKENIZER_CR)
      tokenizer_next();
  } else {
    // TOKENIZER_VARSTRING
    var = tokenizer_variable_num();
    accept(TOKENIZER_VARSTRING);
    accept(TOKENIZER_EQ);
    ubasic_set_string_variable(var, exprs());
    DEBUG_PRINTF("let_statement: assign %s to %d\n", string_variables[var], var);
    if (tokenizer_token() == TOKENIZER_CR)
      tokenizer_next();
  }
}
/*---------------------------------------------------------------------------*/
static void gosub_statement(void) {
  char l[MAX_LABELLEN];
  accept(TOKENIZER_GOSUB);
  DEBUG_PRINTF("Enter gosub_statement\n");
  tokenizer_label(l, MAX_LABELLEN);
  accept(TOKENIZER_LABEL);
  if (tokenizer_token() == TOKENIZER_CR) {
    accept(TOKENIZER_CR);
  }

  if (gosub_stack_ptr < MAX_GOSUB_STACK_DEPTH) {
    gosub_stack[gosub_stack_ptr] = gline_number;
    gosub_stack_ptr++;
    jump_label(l);
  } else {
    printf("Error: gosub stack exhausted\n");
    ubasic_exit(gline_number - 1, "Gosub stack exhausted", "");
  }
}
/*---------------------------------------------------------------------------*/
static void return_statement(void) {
  accept(TOKENIZER_RETURN);
  if (gosub_stack_ptr > 0) {
    gosub_stack_ptr--;
    jump_linenum(gosub_stack[gosub_stack_ptr]);
  } else {
    printf("Error: No matching return on line %d\n", gline_number - 1);
    ubasic_exit(gline_number - 1, "No matching return", 0);
  }
}
/*---------------------------------------------------------------------------*/
static void next_statement(void) {
  int var;

  accept(TOKENIZER_NEXT);
  var = tokenizer_variable_num();
  accept(TOKENIZER_VARIABLE);
  if (for_stack_ptr > 0 && var == for_stack[for_stack_ptr - 1].for_variable) {
    ubasic_set_variable(var, ubasic_get_variable(var) + 1);
    if (ubasic_get_variable(var) <= for_stack[for_stack_ptr - 1].to) {
      jump_linenum(for_stack[for_stack_ptr - 1].line_after_for);
    } else {
      for_stack_ptr--;
      accept(TOKENIZER_CR);
    }
  } else {
    printf("Error: On line %d, unexpected next, no matching for\n",
           gline_number - 1);
    ubasic_exit(gline_number - 1, "Unexpected next, no matching for", "");
  }
}
/*---------------------------------------------------------------------------*/
static void for_statement(void) {
  int for_variable, to;

  accept(TOKENIZER_FOR);
  for_variable = tokenizer_variable_num();
  accept(TOKENIZER_VARIABLE);
  accept(TOKENIZER_EQ);
  ubasic_set_variable(for_variable, expr());
  accept(TOKENIZER_TO);
  to = expr();
  accept(TOKENIZER_CR);

  if (for_stack_ptr < MAX_FOR_STACK_DEPTH) {
    for_stack[for_stack_ptr].line_after_for = gline_number;
    for_stack[for_stack_ptr].for_variable = for_variable;
    for_stack[for_stack_ptr].to = to;
    DEBUG_PRINTF("for_statement: new for at %d, var %d, from %d to %d\n",
                 for_stack[for_stack_ptr].line_after_for,
                 for_stack[for_stack_ptr].for_variable,
                 variables[for_stack[for_stack_ptr].for_variable],
                 for_stack[for_stack_ptr].to);

    for_stack_ptr++;
  } else {
    printf("Error: On line %d, for stack depth exceeded (max: %d)\n",
           gline_number - 1, MAX_FOR_STACK_DEPTH);
    ubasic_exit(gline_number - 1, "for stack depth exceeded", ubasic_exit_static_itoa(MAX_FOR_STACK_DEPTH));
  }
}
/*---------------------------------------------------------------------------*/
static void peek_statement(void) {
  VARIABLE_TYPE peek_addr;
  int var;

  accept(TOKENIZER_PEEK);
  peek_addr = expr();
  accept(TOKENIZER_COMMA);
  var = tokenizer_variable_num();
  accept(TOKENIZER_VARIABLE);
  accept(TOKENIZER_CR);

  ubasic_set_variable(var, peek_function(peek_addr));
}
/*---------------------------------------------------------------------------*/
static void poke_statement(void) {
  VARIABLE_TYPE poke_addr;
  VARIABLE_TYPE value;
  DEBUG_PRINTF("Enter poke_statement\n");
  accept(TOKENIZER_POKE);
  poke_addr = expr();
  accept(TOKENIZER_COMMA);
  value = expr();
  accept(TOKENIZER_CR);

  poke_function(poke_addr, value);
}
/*---------------------------------------------------------------------------*/
static void end_statement(void) {
  accept(TOKENIZER_END);
  ended = 1;
}
/*---------------------------------------------------------------------------*/
static void sleep_statement(void) {
  DEBUG_PRINTF("Enter sleep_statement\n");
  accept(TOKENIZER_SLEEP);

  int sleep_value = expr();
  if (tokenizer_token() == TOKENIZER_CR)
    tokenizer_next();
  sleep_ms(sleep_value * 1000);
}
/*---------------------------------------------------------------------------*/
static void delay_statement(void) {
  DEBUG_PRINTF("Enter delay_statement\n");
  accept(TOKENIZER_DELAY);

  int delay_value = expr();
  if (tokenizer_token() == TOKENIZER_CR)
    tokenizer_next();
  sleep_ms(delay_value);
}
/*---------------------------------------------------------------------------*/
static void randomize_statement(void) {
  DEBUG_PRINTF("Enter randomize_statement\n");
  accept(TOKENIZER_RANDOMIZE);

  int randomize_value = expr();
  if (tokenizer_token() == TOKENIZER_CR)
    tokenizer_next();
  RANDOM_NUM_SEED_x = randomize_value;
}
/*---------------------------------------------------------------------------*/
static void push_statement(void) {
  DEBUG_PRINTF("Enter push_statement\n");
  accept(TOKENIZER_PUSH);

  int push_value = expr();
  if (tokenizer_token() == TOKENIZER_CR)
    tokenizer_next();
  if (int_stack_ptr < MAX_INT_STACK_DEPTH) {
    int_stack[int_stack_ptr] = push_value;
    int_stack_ptr++;
  } else {
    printf("Error: On line %d, integer stack exhausted\n", gline_number - 1);
    ubasic_exit(gline_number - 1, "integer stack exhausted", "");
  }
  DEBUG_PRINTF("Exit push_statement\n");
}
/*---------------------------------------------------------------------------*/
static void pop_statement(void) {
  int var;

  DEBUG_PRINTF("Enter pop_statement\n");
  accept(TOKENIZER_POP);

  if (tokenizer_token() == TOKENIZER_VARIABLE) {
    var = tokenizer_variable_num();
    accept(TOKENIZER_VARIABLE);
    if (int_stack_ptr > 0) {
      int_stack_ptr--;
      ubasic_set_variable(var, int_stack[int_stack_ptr]);
      DEBUG_PRINTF("pop_statement: assign %d to %d\n", variables[var], var);
    } else {
      printf("Error: On line %d, integer stack is empty\n", gline_number - 1);
      ubasic_exit(gline_number - 1, "integer stack is empty", "");
    }
  }
  if (tokenizer_token() == TOKENIZER_CR)
    tokenizer_next();
}
/*---------------------------------------------------------------------------
 * GPIO functions
 *---------------------------------------------------------------------------*/
static void gpio_init_statement(void) {
  DEBUG_PRINTF("Enter gpio_init_statement\n");
  accept(TOKENIZER_GPIOINIT);

  int pin = expr();
  if (tokenizer_token() == TOKENIZER_CR)
    tokenizer_next();
  gpio_init(pin);
}
/*---------------------------------------------------------------------------*/
static void gpio_dir_in_statement(void) {
  DEBUG_PRINTF("Enter gpio_dir_in_statement\n");
  accept(TOKENIZER_GPIODIRIN);

  int pin = expr();
  if (tokenizer_token() == TOKENIZER_CR)
    tokenizer_next();
  gpio_set_dir(pin, GPIO_IN);
}
/*---------------------------------------------------------------------------*/
static void gpio_dir_out_statement(void) {
  DEBUG_PRINTF("Enter gpio_dir_out_statement\n");
  accept(TOKENIZER_GPIODIROUT);

  int pin = expr();
  if (tokenizer_token() == TOKENIZER_CR)
    tokenizer_next();
  gpio_set_dir(pin, GPIO_OUT);
}
/*---------------------------------------------------------------------------*/
static void gpio_on_statement(void) {
  DEBUG_PRINTF("Enter gpio_on_statement\n");
  accept(TOKENIZER_GPIOON);

  int pin = expr();
  if (tokenizer_token() == TOKENIZER_CR)
    tokenizer_next();
  gpio_put(pin, 1);
}
/*---------------------------------------------------------------------------*/
static void gpio_off_statement(void) {
  DEBUG_PRINTF("Enter gpio_off_statement\n");
  accept(TOKENIZER_GPIOOFF);

  int pin = expr();
  if (tokenizer_token() == TOKENIZER_CR)
    tokenizer_next();
  gpio_put(pin, 0);
}
/*---------------------------------------------------------------------------
 * End GPIO functions
 *---------------------------------------------------------------------------*/

static void label_statement(void) {
  char l[MAX_LABELLEN];
  DEBUG_PRINTF("Enter label_statement\n");
  tokenizer_label(l, MAX_LABELLEN);
  DEBUG_PRINTF("Found label %s\n", l);
  index_add_label(gline_number - 1, l);
  accept(TOKENIZER_LABEL);
  accept(TOKENIZER_CR);
  DEBUG_PRINTF("End label_statement\n");
}
/*---------------------------------------------------------------------------*/
static void statement(void) {
  int token;

  token = tokenizer_token();

  switch (token) {
  case TOKENIZER_PRINT:
    print_statement();
    break;
  case TOKENIZER_IF:
    if_statement();
    break;
  case TOKENIZER_GOTO:
    goto_statement();
    break;
  case TOKENIZER_GOSUB:
    gosub_statement();
    break;
  case TOKENIZER_RETURN:
    return_statement();
    break;
  case TOKENIZER_FOR:
    for_statement();
    break;
  case TOKENIZER_PEEK:
    peek_statement();
    break;
  case TOKENIZER_POKE:
    poke_statement();
    break;
  case TOKENIZER_SLEEP:
    sleep_statement();
    break;
  case TOKENIZER_DELAY:
    delay_statement();
    break;
  case TOKENIZER_RANDOMIZE:
    randomize_statement();
    break;
  case TOKENIZER_POP:
    pop_statement();
    break;
  case TOKENIZER_PUSH:
    push_statement();
    break;
  case TOKENIZER_OS:
    os_statement();
    break;
  case TOKENIZER_NEXT:
    next_statement();
    break;
  case TOKENIZER_END:
    end_statement();
    break;
  case TOKENIZER_LABEL:
    label_statement();
    break;
  case TOKENIZER_GPIOINIT:
    gpio_init_statement();
    break;
  case TOKENIZER_GPIODIRIN:
    gpio_dir_in_statement();
    break;
  case TOKENIZER_GPIODIROUT:
    gpio_dir_out_statement();
    break;
  case TOKENIZER_GPIOON:
    gpio_on_statement();
    break;
  case TOKENIZER_GPIOOFF:
    gpio_off_statement();
    break;
  case TOKENIZER_LET:
    accept(TOKENIZER_LET);
    /* Fall through. */
  case TOKENIZER_VARIABLE:
  case TOKENIZER_VARFLOAT:
  case TOKENIZER_VARSTRING:
    let_statement();
    break;
  default:
    printf("Error: On line %d, unknown statement(): %d\n", gline_number - 1,
           token);
    ubasic_exit(gline_number, "unknown statement()", "");
  }
}
/*---------------------------------------------------------------------------*/
static void line_statement(void) {
  // DEBUG_PRINTF("----------- Line number %d ---------\n", tokenizer_num());
  // index_add(tokenizer_num(), tokenizer_pos());
  // accept(TOKENIZER_NUMBER);
  int cline_number = linenum_find_by_pos(tokenizer_pos());
  if (cline_number < 0) {
    DEBUG_PRINTF("----------- New Line number %d ---------\n", gline_number);
    index_add(gline_number++, tokenizer_pos());
  } else {
    DEBUG_PRINTF("----------- Line number %d ---------\n", cline_number);
    gline_number = cline_number + 1;
  }

  statement();
  return;
}
/*---------------------------------------------------------------------------*/
void ubasic_run(void) {
  if (tokenizer_finished()) {
    DEBUG_PRINTF("uBASIC program finished\n");
    return;
  }

  line_statement();
  check_if_should_enter_CMD_mode();
}
/*---------------------------------------------------------------------------*/
int ubasic_finished(void) { return ended || tokenizer_finished(); }
/*---------------------------------------------------------------------------*/
void ubasic_set_variable(int varnum, VARIABLE_TYPE value) {
  if (varnum >= 0 && varnum <= MAX_VARNUM) {
    variables[varnum] = value;
  }
}
/*---------------------------------------------------------------------------*/
VARIABLE_TYPE
ubasic_get_variable(int varnum) {
  if (varnum >= 0 && varnum <= MAX_VARNUM) {
    return variables[varnum];
  }
  return 0;
}
/*---------------------------------------------------------------------------*/
void ubasic_set_float_variable(int varnum, VARFLOAT_TYPE value) {
  if (varnum >= 0 && varnum <= MAX_VARNUM) {
    float_variables[varnum] = value;
  }
}
/*---------------------------------------------------------------------------*/
VARFLOAT_TYPE ubasic_get_float_variable(int varnum) {
  if (varnum >= 0 && varnum <= MAX_VARNUM) {
    return float_variables[varnum];
  }
  return 0;
}
/*---------------------------------------------------------------------------*/
void ubasic_set_string_variable(int varnum, VARSTRING_TYPE value) {
  if (varnum >= 0 && varnum <= MAX_VARNUM) {
    if(string_variables[varnum]!=NULL)
      free(string_variables[varnum]);
    string_variables[varnum] = malloc(strlen(value) + 1);
    strcpy(string_variables[varnum], value);
  }
}
/*---------------------------------------------------------------------------*/
VARSTRING_TYPE ubasic_get_string_variable(int varnum) {
  if (varnum >= 0 && varnum <= MAX_VARNUM) {
    return string_variables[varnum];
  }
  return 0;
}
