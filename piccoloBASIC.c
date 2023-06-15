/*
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// #include <unistd.h>

#include "pico/stdlib.h"

#include "lfs_wrapper.h"
#include "piccoloBASIC.h"
#include "ubasic.h"

#define MAX_CMD_LINE 100
#define MAX_PATH_LEN 100

char cwd[MAX_PATH_LEN];

static char *getLine() {
  const uint startLineLength = 8; // the linebuffer will automatically grow for longer lines
  const char eof = 255; // EOF in stdio.h -is -1, but getchar returns int 255 to
                        // avoid blocking

  char *pStart = (char *)malloc(startLineLength);
  char *pPos = pStart;             // next character position
  size_t maxLen = startLineLength; // current max buffer size
  size_t len = maxLen;             // current max length
  int c;

  if (!pStart) {
    return NULL; // out of memory or dysfunctional heap
  }

  while (1) {
    c = getchar();   // expect next character entry
    printf("%c", c);
    if (c == 0x03) { // CTRL-C
      if (!pStart) {
        free(pStart);
      }
      return NULL;
    }
    if (c == '\r') {
      continue; // ignore
    }

    if (c == eof || c == '\n') {
      break; // non blocking exit
    }

    if (--len == 0) { // allow larger buffer
      len = maxLen;
      // double the current line buffer size
      char *pNew = (char *)realloc(pStart, maxLen *= 2);
      if (!pNew) {
        free(pStart);
        return NULL; // out of memory abort
      }
      // fix pointer for new buffer
      pPos = pNew + (pPos - pStart);
      pStart = pNew;
    }

    // stop reading if lineBreak character entered
    if ((*pPos++ = c) == '\n') {
      break;
    }
  }

  *pPos = '\0'; // set string end mark
  return pStart;
}

int enter_CMD_mode() {
  char line[MAX_CMD_LINE];
  char *result;
  int done = 0;

  printf("PiccoloBASIC CMD Mode\n");
  sprintf(cwd, "%s", "/");

  while (!done) {
    result = getLine('\r');
    printf("CMD: %s\n", result);
    // Extract the first token
    char *token = strtok(result, " ");

    if (token != NULL) {
      printf("CMD: %s\n", result);
      if (strcmp(token, "exit") == 0) {
        done = 1;
      }
      if (strcmp(token, "ls") == 0) {
        lfswrapper_dump_dir(cwd);
      }
      if (strcmp(token, "cd") == 0) {
        token = strtok(NULL, " ");
        if ((strcmp(token, "..") == 0) || (strcmp(token, "/") == 0)) {
          sprintf(cwd, "//", token);
        } else if ((strcmp(token, ".") == 0)) {
          sprintf(cwd, "%s", cwd);
        } else {
          sprintf(cwd, "/%s", token);
        }
      }
    }
  }
}

  int check_if_should_enter_CMD_mode() {
    int chr = getchar_timeout_us(0);
    if (chr != PICO_ERROR_TIMEOUT) {
      if (chr == 3) { // CTRL-C
        int chr2 = getchar_timeout_us(500 * 1000);
        if (chr2 != PICO_ERROR_TIMEOUT) {
        enter_CMD_mode();
        return 1;
        }
      }
    }

    return 0;
  }

  int main(int argc, char *argv[]) {
    stdio_init_all();

sleep_ms(1000);

    lfswrapper_lfs_mount();
    lfswrapper_dump_dir("/");

    static const char fakeprogram[] = "for i = 1 to 10\n\
print i\n\
next i\n\
let x = 99\n\
print x\n\
print \"The end is nigh\"\n\
print \"The end is here!\"";
    ubasic_init(fakeprogram);

    do {
      ubasic_run();
    } while (!ubasic_finished());

    // Never actually return/exit
    while (true) {
      check_if_should_enter_CMD_mode();
      printf("Hello, world!\n");
      sleep_ms(1000);
    }
    return 0;
    // -------------------------
    // Open the file
    char *filename = argv[1];
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
      perror("Error opening file");
      return 1;
    }

    // Allocate memory for the string
    char *program = malloc(PROG_BUFFER_SIZE);
    if (program == NULL) {
      perror("Error allocating memory for string");
      return 1;
    }

    // Read the file into the string
    size_t program_size = 0; // Keep track of the size of the string
    while (fgets(program + program_size, PROG_BUFFER_SIZE, file) != NULL) {
      program_size += strlen(program + program_size);
      program = realloc(program, program_size + PROG_BUFFER_SIZE);
      if (program == NULL) {
        perror("Error reallocating memory for string");
        return 1;
      }
    }

    // Close the file
    fclose(file);

    ubasic_init(program);

    do {
      ubasic_run();
    } while (!ubasic_finished());

    // Free the memory allocated for the string
    free(program);

    // Never actually return/exit
    while (true) {
      printf("Hello, world!\n");
      sleep_ms(1000);
    }
    return 0;
  }
