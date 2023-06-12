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
#include "pico/stdlib.h"

#include "piccoloBASIC.h"
#include "ubasic.h"

int main(int argc, char *argv[]) {
  stdio_init_all();

	static const char fakeprogram[] =
"if 5<7 then print 111 else print 0\n\
gosub sub:\n\
for i = 1 to 10\n\
print i\n\
// print i*2\n\
next i\n\
print \"end\"\n\
end\n\
sub:\n\
print \"subroutine\"\n\
return\n";

	ubasic_init(fakeprogram);

	do {
    		ubasic_run();
  	} while (!ubasic_finished());

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

  return 0;
}
