#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

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

void doupload(const char *filename) {
  FILE *file = fopen(filename, "r");
  if (file == NULL) {
    return;
  }

  int c;
  while ((c = fgetc(file)) != EOF) {
    // Print the character in hex
    printf("%d\n", c);
  }

  fclose(file);
}

int get_filesize(char *file_name)
{
    FILE* fp = fopen(file_name, "r"); 
    if (fp == NULL) {
        return -1;
    }
  
    fseek(fp, 0L, SEEK_END);
    int sz = (int) ftell(fp);
    fclose(fp);
  
    return sz;
}

static char *getLine(int fd, int echo) {
  const uint startLineLength = 8; // the linebuffer will automatically grow
  // const char eof = 255; // EOF in stdio.h is -1, but getchar returns int 255
  // to avoid blocking

  char *pStart = (char *)malloc(startLineLength);
  char *pPos = pStart;             // next character position
  size_t maxLen = startLineLength; // current max buffer size
  size_t len = maxLen;             // current max length
  int c;

  if (!pStart) {
    return NULL; // out of memory or dysfunctional heap
  }

  while (1) {

    int sts = read(fd, &c, 1);
    while (sts == -1) {
      usleep(500);
      sts = read(fd, &c, 1);
    }

    // c = fgetc(fp); // expect next character entry
    if ((echo) && (c >= ' ') && (c <= 126))
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

    // if (c == eof || c == '\n') {
    if (c == '\n') {
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
  if (echo)
    printf("\n");
  return pStart;
}

int send_cmd(int fd, char *cmd) {
    return write(fd, cmd, strlen(cmd));
}

int main(int argc, char *argv[]) {
  FILE *fpin;
  int pb;
  char ch;

  if (argc != 3) {
    printf("Usage: %s <filename> <device>\n", argv[0]);
    printf("eg: %s main.bas /dev/ttyACM0\n", argv[0]);
    return 1;
  }

  printf("Uploading %s to piccoloBASIC via %s\n", argv[1], argv[2]);

  pb = open(argv[2], O_RDWR | O_NOCTTY | O_NONBLOCK);
  if (pb == -1) {
    printf("Couldn't open USB or serial port to piccoloBASIC!");
    return 1;
  }
  printf("Device opened...\n");
  // Send CTRL-C twice
  char ctrlc = 0x03;
  // fputc(0x03, fpout);
  write(pb, &ctrlc, 1);
  fsync(pb);
  write(pb, &ctrlc, 1);
  fsync(pb);
  char *banner = getLine(pb, 1);
  if (strcmp(banner, "PiccoloBASIC CMD Mode") != 0) {
    printf("Error entering CMD mode.\n");
    free(banner);
    close(pb);
    return 1;
  }
  free(banner);
  int uploadfilesize = get_filesize(argv[1]);
  if (uploadfilesize == -1) {
    printf("Can't find file %s\n", argv[1]);
  } else {
    char uploadcmd[128];
    sprintf(uploadcmd,"upload %s %d\r", argv[1], uploadfilesize);
    send_cmd(pb, uploadcmd);
    doupload(argv[1]);
  }
  send_cmd(pb, "exit\r");
  // read_file_hex(argv[1]);
  close(pb);
  return 0;
}