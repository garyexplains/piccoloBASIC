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

/*
 * Warning: This code needs more error checking for bad/malformed commands sent
 * in CMD mode
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// #include <unistd.h>

#include "hardware/watchdog.h"
#include "pico/stdlib.h"

#include "lfs_wrapper.h"
#include "piccoloBASIC.h"
#include "ubasic.h"

#define MAX_CMD_LINE 100
#define MAX_PATH_LEN 100

/*
 * Eek! Globals!
 */
char cwd[MAX_PATH_LEN];
int lookahead = -1;
int needsreboot = 0;

static char *getLine(int echo) {
  const uint startLineLength =
      8; // the linebuffer will automatically grow for longer lines
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
    if (lookahead >= 0) {
      c = lookahead;
      lookahead = -1;
    } else {
      c = getchar();
    }
    if ((echo) && (c >= ' ') && (c <= 126))
      printf("%c", c);
    if (c == 0x03) { // CTRL-C
      if (!pStart) {
        free(pStart);
      }
      return NULL;
    }

    if ((char)c == eof) {
      break; // Done
    }

    if ((char)c == '\n') {
      break; // Done
    }

    if ((char)c == '\r') {
      lookahead = getchar_timeout_us(500000);
      if (lookahead == PICO_ERROR_TIMEOUT) {
        // Assume \r was the end of the line
        break; // Done
      }
      if (lookahead == '\n') {
        lookahead = -1;
        break;
      } else {
        // Assume \r was the end of the line
        break; // Done
      }
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
    *pPos++ = c;
  }

  *pPos = '\0'; // set string end mark
  if (echo)
    printf("\n");
  return pStart;
}

int doupload(char *uploadfilename, int uploadfilesize) {
  int count = 0;
  char *program = malloc(PROG_BUFFER_SIZE);
  if (program == NULL) {
    return -1;
  }

  lfswrapper_file_open(uploadfilename, LFS_O_RDWR | LFS_O_CREAT | LFS_O_TRUNC);
  while (count < uploadfilesize) {
    char *result = getLine(0);
    printf("+OK\n");
    stdio_flush();
    int b = atoi(result);
    program[count++] = (char)b;
    free(result);
  }
  lfswrapper_file_write(program, uploadfilesize);
  lfswrapper_file_close();
  return count;
}

int enter_CMD_mode() {
  char line[MAX_CMD_LINE];
  char *result = NULL;
  int done = 0;

  stdio_flush();
  printf("+OK PiccoloBASIC CMD Mode\n");
  stdio_flush();

  sprintf(cwd, "%s", "/");

  while (!done) {
    if (result != NULL)
      free(result);
    result = getLine(0);
    // Extract the first token
    char *token = strtok(result, " ");

    if (token != NULL) {
      if (strcmp(token, "exit") == 0) {
        if (needsreboot)
          watchdog_reboot(0, SRAM_END, 0);
        done = 1;
      } else if (strcmp(token, "reboot") == 0) {
        watchdog_reboot(0, SRAM_END, 0);
      } else if (strcmp(token, "ls") == 0) {
        printf("+OK\n");
        lfswrapper_dump_dir(cwd);
      } else if (strcmp(token, "upload") == 0) { // upload main.bas 432
        printf("+OK\n");
        token = strtok(NULL, " "); // filename
        char *uploadfilename = malloc(strlen(token) + 1);
        strcpy(uploadfilename, token);
        token = strtok(NULL, " "); // file size in bytes
        int uploadfilesize = atoi(token);
        if (uploadfilesize > 0) {
          doupload(uploadfilename, uploadfilesize);
        }
        free(uploadfilename);
        needsreboot = 1;
      } else if (strcmp(token, "rm") == 0) { // upload main.bas 432
        printf("+OK\n");
        token = strtok(NULL, " "); // filename
        lfswrapper_delete_file(token);
      } else if (strcmp(token, "cd") == 0) {
        printf("+OK\n");
        token = strtok(NULL, " ");
        if ((strcmp(token, "..") == 0) || (strcmp(token, "/") == 0)) {
          sprintf(cwd, "//", token);
        } else if ((strcmp(token, ".") == 0)) {
          sprintf(cwd, "%s", cwd);
        } else {
          sprintf(cwd, "/%s", token);
        }
      } else {
        printf("-ERR %s\n", token);
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
  int proglen;
  bool norun = false;

  stdio_init_all();

  // Check if GPI10 is high, if so don't run program
  // This allows the uploader to enter CMD mode so
  // it still possible to upload a new BASIC program
  gpio_init(10);
  gpio_set_dir(10, GPIO_IN);
  gpio_pull_down(10);
  norun = gpio_get(10);

  lfswrapper_lfs_mount();

  if (!norun) {
    // Allocate memory for the program
    char *program = malloc(PROG_BUFFER_SIZE);
    if (program == NULL) {
      perror("Error allocating memory for program");
      return 1;
    }

    int mainsz = lfswrapper_get_file_size("main.bas");

    if (mainsz > 0) {
      lfswrapper_file_open("main.bas", LFS_O_RDONLY);
      proglen = lfswrapper_file_read(program, PROG_BUFFER_SIZE);
      program[proglen] = 0;
      lfswrapper_file_close();
    }
    ubasic_init(program);
    do {
      ubasic_run();
    } while (!ubasic_finished());

    // Free the memory allocated for the program
    free(program);
  } else {
    // Eek! Hardcoded!
    gpio_init(14);
    gpio_set_dir(14, GPIO_OUT);
    gpio_put(14, 1);
  }
  // Never actually return/exit
  while (true) {
    check_if_should_enter_CMD_mode();
    sleep_ms(500);
  }
  return 0;
}
