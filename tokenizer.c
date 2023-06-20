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
/* */
/*
 * Convert a string to an intmax_t
 *
 * Ignores `locale' stuff.  Assumes that the upper and lower case
 * alphabets and digits are each contiguous.
 */
VARIABLE_TYPE 
strtoi_VARIABLE_TYPE(const char *nptr, char **endptr, int base)
{
	const char *s;
	intmax_t acc, cutoff;
	int c;
	int neg, any, cutlim;
	/*
	 * Skip white space and pick up leading +/- sign if any.
	 * If base is 0, allow 0x for hex and 0 for octal, else
	 * assume decimal; if base is already 16, allow 0x.
	 */
	s = nptr;
	do {
		c = (unsigned char) *s++;
	} while (isspace(c));
	if (c == '-') {
		neg = 1;
		c = *s++;
	} else {
		neg = 0;
		if (c == '+')
			c = *s++;
	}
	if ((base == 0 || base == 16) &&
	    c == '0' && (*s == 'x' || *s == 'X')) {
		c = s[1];
		s += 2;
		base = 16;
	}
	if (base == 0)
		base = c == '0' ? 8 : 10;
	/*
	 * Compute the cutoff value between legal numbers and illegal
	 * numbers.  That is the largest legal value, divided by the
	 * base.  An input number that is greater than this value, if
	 * followed by a legal input character, is too big.  One that
	 * is equal to this value may be valid or not; the limit
	 * between valid and invalid numbers is then based on the last
	 * digit.  For instance, if the range for intmax_t is
	 * [-9223372036854775808..9223372036854775807] and the input base
	 * is 10, cutoff will be set to 922337203685477580 and cutlim to
	 * either 7 (neg==0) or 8 (neg==1), meaning that if we have
	 * accumulated a value > 922337203685477580, or equal but the
	 * next digit is > 7 (or 8), the number is too big, and we will
	 * return a range error.
	 *
	 * Set any if any `digits' consumed; make it negative to indicate
	 * overflow.
	 */
	/* BIONIC: avoid division and module for common cases */
#define  CASE_BASE(x) \
            case x:  \
	        if (neg) { \
                    cutlim = INTMAX_MIN % x; \
		    cutoff = INTMAX_MIN / x; \
	        } else { \
		    cutlim = INTMAX_MAX % x; \
		    cutoff = INTMAX_MAX / x; \
		 }; \
		 break
		 
	switch (base) {
            case 4:
                if (neg) {
                    cutlim = (int)(INTMAX_MIN % 4);
                    cutoff = INTMAX_MIN / 4;
                } else {
                    cutlim = (int)(INTMAX_MAX % 4);
                    cutoff = INTMAX_MAX / 4;
                }
                break;
	    CASE_BASE(8);
	    CASE_BASE(10);
	    CASE_BASE(16);
	    default:  
	              cutoff  = neg ? INTMAX_MIN : INTMAX_MAX;
		      cutlim  = cutoff % base;
	              cutoff /= base;
	}
#undef CASE_BASE
	
	if (neg) {
		if (cutlim > 0) {
			cutlim -= base;
			cutoff += 1;
		}
		cutlim = -cutlim;
	}
	for (acc = 0, any = 0;; c = (unsigned char) *s++) {
		if (isdigit(c))
			c -= '0';
		else if (isalpha(c))
			c -= isupper(c) ? 'A' - 10 : 'a' - 10;
		else
			break;
		if (c >= base)
			break;
		if (any < 0)
			continue;
		if (neg) {
			if (acc < cutoff || (acc == cutoff && c > cutlim)) {
				any = -1;
				acc = INTMAX_MIN;
				//errno = ERANGE;
			} else {
				any = 1;
				acc *= base;
				acc -= c;
			}
		} else {
			if (acc > cutoff || (acc == cutoff && c > cutlim)) {
				any = -1;
				acc = INTMAX_MAX;
				//errno = ERANGE;
			} else {
				any = 1;
				acc *= base;
				acc += c;
        printf("acc: %ld\n", acc);
			}
		}
	}
	if (endptr != 0)
		*endptr = (char *) (any ? s - 1 : nptr);
	return (acc);
}
/* */
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
    {"pininit", TOKENIZER_GPIOINIT},      {"pindirin", TOKENIZER_GPIODIRIN},
    {"pindirout", TOKENIZER_GPIODIROUT},  {"pinon", TOKENIZER_GPIOON},
    {"pinoff", TOKENIZER_GPIOOFF},
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
VARIABLE_TYPE tokenizer_num(void) {
  VARIABLE_TYPE i;
  //i = atol(ptr);
  //i = strtol(ptr, NULL, 10);
  i = strtoi_VARIABLE_TYPE(ptr, NULL, 0);
  long j = 3489660952;
  j++;
  printf("%ld\n", j);
  DEBUG_PRINTF("tokenizer_num result: %ld\n", i);
  return i;
}
/*---------------------------------------------------------------------------*/
  VARFLOAT_TYPE tokenizer_numfloat(void) {
    return atof(ptr);
  }
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
