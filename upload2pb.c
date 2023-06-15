#include <stdio.h>
#include <stdlib.h>

/*
 * Compile with:
 * gcc -o upload2pb upload2pb.c
*/

char *file2buffer(const char *filename) {
    FILE *file = fopen(filename, "rb");
    if (file == NULL) {
        return NULL;
    }

    fseek(file, 0L, SEEK_END);
    long size = ftell(file);
    rewind(file);

    char *buffer = malloc(size + 1);
    if (buffer == NULL) {
        fclose(file);
        return NULL;
    }

    fread(buffer, size, 1, file);
    buffer[size] = '\0';

    fclose(file);

    return buffer;
}

void read_file_hex(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        return;
    }

    int c;
    while ((c = fgetc(file)) != EOF) {
        // Print the character in hex
        printf("%c %02X\n", c, c);
    }

    fclose(file);
}

static char *getLine(FILE *fp, int echo) {
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
    c = fgetc(fp);   // expect next character entry
    if( (echo) && (c>=' ') && (c<=126) )
      printf("%c", c);
    if (c == 0x03) { // CTRL-C
      if (!pStart) {
        free(pStart);
      }
      return NULL;
    }
    if (c == '\n') {
      continue; // ignore
    }

    if (c == eof || c == '\r') {
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
    if ((*pPos++ = c) == '\r') {
      break;
    }
  }

  *pPos = '\0'; // set string end mark
  if (echo)
    printf("\n");
  return pStart;
}

int main(int argc, char *argv[]) {
    FILE *fpin;
    FILE *fpout;
    char ch;

    if (argc != 3) {
        printf("Usage: %s <filename> <device>\n", argv[0]);
        printf("eg: %s main.bas /dev/ttyACM0\n", argv[0]);
        return 1;
    }

    fpout = fopen(argv[2], "rw");
    if (fpout == NULL) {
        printf("Couldn't open USB or serial port to piccoloBASIC!");
        return 1;
    }

    // Send CTRL-C twice
    fputc(0x03, fpout);
    fputc(0x03, fpout);

    char *banner = getLine(fpout, 0);
    printf("R: %s\n", banner);
    free(banner);
    // read_file_hex(argv[1]);

    return 0;
}