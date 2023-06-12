/*
 * Copyright (c) 2006, Adam Dunkels
 * Copyright (c) 2023, Gary Sims
 * All rights reserved.
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

#include "tokenizer.h"
#include <ctype.h>
#include <stdio.h> /* printf() */
#include <stdlib.h>
#include <string.h>

#define DEBUG 0

#if DEBUG
#define DEBUG_PRINTF(...) printf(__VA_ARGS__)
#else
#define DEBUG_PRINTF(...)
#endif

static char const *ptr, *nextptr;

#define MAX_NUMLEN 18

struct keyword_token {
  char *keyword;
  int token;
};

static int current_token = TOKENIZER_ERROR;

static const struct keyword_token keywords[] = {
    {"let", TOKENIZER_LET},       {"print", TOKENIZER_PRINT},
    {"if", TOKENIZER_IF},         {"then", TOKENIZER_THEN},
    {"else", TOKENIZER_ELSE},     {"for", TOKENIZER_FOR},
    {"to", TOKENIZER_TO},         {"next", TOKENIZER_NEXT},
    {"goto", TOKENIZER_GOTO},     {"gosub", TOKENIZER_GOSUB},
    {"return", TOKENIZER_RETURN}, {"call", TOKENIZER_CALL},
    {"rem", TOKENIZER_REM},       {"peek", TOKENIZER_PEEK},
    {"poke", TOKENIZER_POKE},     {"end", TOKENIZER_END},
    {"delay", TOKENIZER_DELAY},   {"sleep", TOKENIZER_SLEEP},
    {"zero", TOKENIZER_ZERO},     {"not", TOKENIZER_NOT},
    {"randomize", TOKENIZER_RANDOMIZE},     {"randint", TOKENIZER_RANDINT},
    {"rnd", TOKENIZER_RND},       {"time", TOKENIZER_TIME},
    {"push", TOKENIZER_PUSH},     {"pop", TOKENIZER_POP},
    {"abs", TOKENIZER_ABS},      {"atn", TOKENIZER_ATN},
    {"cos", TOKENIZER_COS},      {"exp", TOKENIZER_EXP},
    {"log", TOKENIZER_LOG},      {"tan", TOKENIZER_TAN},
    {"sin", TOKENIZER_SIN},      {"sqr", TOKENIZER_SQR},
    {"len", TOKENIZER_LEN},      {"os", TOKENIZER_OS},
    {"//", TOKENIZER_REM},
    {NULL, TOKENIZER_ERROR}};

/*---------------------------------------------------------------------------*/
static int singlechar(void) {
  if (*ptr == '\n') {
    return TOKENIZER_CR;
  } else if (*ptr == ',') {
    return TOKENIZER_COMMA;
  } else if (*ptr == ';') {
    return TOKENIZER_SEMICOLON;
  } else if (*ptr == '+') {
    return TOKENIZER_PLUS;
  } else if (*ptr == '-') {
    return TOKENIZER_MINUS;
  } else if (*ptr == '&') {
    return TOKENIZER_AND;
  } else if (*ptr == '|') {
    return TOKENIZER_OR;
  } else if (*ptr == '*') {
    return TOKENIZER_ASTR;
  } else if (*ptr == '/') {
    if ((*(ptr + 1) != 0) && (*(ptr + 1) == '/'))
      return 0;
    else
      return TOKENIZER_SLASH;
  } else if (*ptr == '%') {
    return TOKENIZER_MOD;
  } else if (*ptr == '(') {
    return TOKENIZER_LEFTPAREN;
  } else if (*ptr == '#') {
    return TOKENIZER_HASH;
  } else if (*ptr == ')') {
    return TOKENIZER_RIGHTPAREN;
  } else if (*ptr == '<') {
    return TOKENIZER_LT;
  } else if (*ptr == '>') {
    return TOKENIZER_GT;
  } else if (*ptr == '=') {
    return TOKENIZER_EQ;
  }
  return 0;
}
/*---------------------------------------------------------------------------*/
int isfloatdigit(char c) {
  return (((c >= '0' && c <= '9') || (c == '.')) ? 1 : 0);
}
/*---------------------------------------------------------------------------*/
static int get_next_token(void) {
  struct keyword_token const *kt;
  int i;
  int isfloat = 0;

  DEBUG_PRINTF("get_next_token(): '%s'\n", ptr);

  if (*ptr == 0) {
    return TOKENIZER_ENDOFINPUT;
  }

  if (isdigit(*ptr)) {
    for (i = 0; i < MAX_NUMLEN; ++i) {
      if (!isfloatdigit(ptr[i])) {
        if (i > 0) {
          nextptr = ptr + i;
          if (isfloat)
            return TOKENIZER_NUMFLOAT;
          else
            return TOKENIZER_NUMBER;
        } else {
          printf("Number is too short\n");
          exit(-1);
        }
      }
      if (ptr[i] == '.')
        isfloat = 1;
      if (!isfloatdigit(ptr[i])) {
        DEBUG_PRINTF("Malformed number\n");
        exit(-1);
      }
    }
    printf("Number is too long\n");
    exit(-1);
  } else if (singlechar()) {
    nextptr = ptr + 1;
    return singlechar();
  } else if (*ptr == '"') {
    nextptr = ptr;
    do {
      ++nextptr;
    } while (*nextptr != '"');
    ++nextptr;
    return TOKENIZER_STRING;
  } else {
    for (kt = keywords; kt->keyword != NULL; ++kt) {
      if (strncmp(ptr, kt->keyword, strlen(kt->keyword)) == 0) {
        nextptr = ptr + strlen(kt->keyword);
        return kt->token;
      }
    }
  }

  // Is it a label?
  nextptr = ptr;
  do {
    ++nextptr;
  } while ((*nextptr != '\n') && (*nextptr != ':') &&
           (nextptr - ptr < MAX_LABELLEN));

  if (*nextptr == ':') {
    ++nextptr;
    return TOKENIZER_LABEL;
  }

  // Floasting point variable?
  if ((*ptr >= 'a' && *ptr <= 'z') && (*(ptr + 1) == '#')) {
    nextptr = ptr + 2;
    return TOKENIZER_VARFLOAT;
  }

  // String variable?
  if ((*ptr >= 'a' && *ptr <= 'z') && (*(ptr + 1) == '$')) {
    nextptr = ptr + 2;
    return TOKENIZER_VARSTRING;
  }

  // Integer variable?
  if (*ptr >= 'a' && *ptr <= 'z') {
    nextptr = ptr + 1;
    return TOKENIZER_VARIABLE;
  }

  return TOKENIZER_ERROR;
}
/*---------------------------------------------------------------------------*/
void tokenizer_goto(const char *program) {
  ptr = program;
  current_token = get_next_token();
}
/*---------------------------------------------------------------------------*/
void tokenizer_init(const char *program) {
  tokenizer_goto(program);
  current_token = get_next_token();
}
/*---------------------------------------------------------------------------*/
int tokenizer_token(void) { return current_token; }
/*---------------------------------------------------------------------------*/
void tokenizer_next(void) {

  if (tokenizer_finished()) {
    return;
  }

  DEBUG_PRINTF("tokenizer_next: %p\n", nextptr);
  ptr = nextptr;

  while (*ptr == ' ') {
    ++ptr;
  }
  current_token = get_next_token();

  if (current_token == TOKENIZER_REM) {
    while (!(*nextptr == '\n' || tokenizer_finished())) {
      ++nextptr;
    }
    if (*nextptr == '\n') {
      ++nextptr;
    }
    tokenizer_next();
  }

  DEBUG_PRINTF("tokenizer_next: '%s' %d\n", ptr, current_token);
  return;
}
/*---------------------------------------------------------------------------*/
VARIABLE_TYPE tokenizer_num(void) { return atoi(ptr); }
/*---------------------------------------------------------------------------*/
VARFLOAT_TYPE tokenizer_numfloat(void) { return atof(ptr); }
/*---------------------------------------------------------------------------*/
void tokenizer_string(char *dest, int len) {
  char *string_end;
  int string_len;

  if (tokenizer_token() != TOKENIZER_STRING) {
    printf("Internal error, expecting string\n");
    exit(-1);
  }
  string_end = strchr(ptr + 1, '"');
  if (string_end == NULL) {
    printf("Error: Missing quote\n");
    exit(-1);
  }
  string_len = string_end - ptr - 1;
  if (len < string_len) {
    string_len = len;
  }

  if (string_len >= len) {
    printf("Error: String too long\n");
    exit(-1);
  }
  memcpy(dest, ptr + 1, string_len);
  dest[string_len] = 0;
}
/*---------------------------------------------------------------------------*/
void tokenizer_label(char *dest, int len) {
  char *string_end;
  int string_len;

  DEBUG_PRINTF("tokenizer_label ptr is: '%s'\n", ptr);
  string_end = strchr(ptr + 1, ':');
  if (string_end == NULL) {
    printf("Internal error, no : found in label\n");
    exit(-1);
  }

  DEBUG_PRINTF("tokenizer_label string_end is: '%s'\n", string_end);

  string_len = string_end - ptr;
  DEBUG_PRINTF("tokenizer_label string_len is: '%d'\n", string_len);
  if (len < string_len) {
    string_len = len;
  }
  if (string_len >= len) {
    printf("Error: Label too long\n");
    exit(-1);
  }
  memcpy(dest, ptr, string_len);
  dest[string_len] = 0;
}
/*---------------------------------------------------------------------------*/
void tokenizer_error_print(int line, char *msg) {
  printf("Error on line %d: %s\n", line, msg);
}
/*---------------------------------------------------------------------------*/
int tokenizer_finished(void) {
  return *ptr == 0 || current_token == TOKENIZER_ENDOFINPUT;
}
/*---------------------------------------------------------------------------*/
int tokenizer_variable_num(void) { return *ptr - 'a'; }
/*---------------------------------------------------------------------------*/
char const *tokenizer_pos(void) { return ptr; }
