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
#ifndef __TOKENIZER_H__
#define __TOKENIZER_H__

#include "vartype.h"

#define MAX_LABELLEN 16

enum {
  TOKENIZER_ERROR,
  TOKENIZER_ENDOFINPUT,
  TOKENIZER_NUMBER,
  TOKENIZER_NUMFLOAT,
  TOKENIZER_STRING,
  TOKENIZER_VARIABLE,
  TOKENIZER_VARFLOAT,
  TOKENIZER_VARSTRING,
  TOKENIZER_LABEL,
  TOKENIZER_LET,
  TOKENIZER_PRINT,
  TOKENIZER_IF,
  TOKENIZER_THEN,
  TOKENIZER_ELSE,
  TOKENIZER_FOR,
  TOKENIZER_TO,
  TOKENIZER_NEXT,
  TOKENIZER_GOTO,
  TOKENIZER_GOSUB,
  TOKENIZER_RETURN,
  TOKENIZER_CALL,
  TOKENIZER_REM,
  TOKENIZER_PEEK,
  TOKENIZER_POKE,
  TOKENIZER_DELAY,
  TOKENIZER_SLEEP,
  TOKENIZER_RANDOMIZE,
  TOKENIZER_PUSH,
  TOKENIZER_POP,
  TOKENIZER_END,
  TOKENIZER_BUILTINS__START,
  TOKENIZER_ZERO,
  TOKENIZER_NOT,
  TOKENIZER_RANDINT,
  TOKENIZER_TIME,
  TOKENIZER_BUILTINS__END,
  TOKENIZER_BUILTINSF__START,
  TOKENIZER_RND,
  TOKENIZER_ABS,
  TOKENIZER_ATN,
  TOKENIZER_COS,
  TOKENIZER_EXP,
  TOKENIZER_LOG,
  TOKENIZER_SIN,
  TOKENIZER_SQR,
  TOKENIZER_TAN,
  TOKENIZER_BUILTINSF__END,
  TOKENIZER_LEN,
  TOKENIZER_OS,
  TOKENIZER_COMMA,
  TOKENIZER_SEMICOLON,
  TOKENIZER_PLUS,
  TOKENIZER_MINUS,
  TOKENIZER_AND,
  TOKENIZER_OR,
  TOKENIZER_ASTR,
  TOKENIZER_SLASH,
  TOKENIZER_MOD,
  TOKENIZER_HASH,
  TOKENIZER_LEFTPAREN,
  TOKENIZER_RIGHTPAREN,
  TOKENIZER_LT,
  TOKENIZER_GT,
  TOKENIZER_EQ,
  TOKENIZER_CR,
};

void tokenizer_goto(const char *program);
void tokenizer_init(const char *program);
void tokenizer_next(void);
int tokenizer_token(void);
VARIABLE_TYPE tokenizer_num(void);
VARFLOAT_TYPE tokenizer_numfloat(void);
int tokenizer_variable_num(void);
void tokenizer_string(char *dest, int len);
void tokenizer_label(char *dest, int len);

int tokenizer_finished(void);
void tokenizer_error_print(int line, char *msg);

char const *tokenizer_pos(void);

#endif /* __TOKENIZER_H__ */
