#include <stdio.h>

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
        printf("%02X ", c);
    }

    fclose(file);
}

int main(int argc, char *argv[]) {
    FILE *fp;
    char ch;

    if (argc != 3) {
        printf("Usage: %s <filename> <device>\n", argv[0]);
        printf("eg: %s main.bas /dev/ttyACM0\n", argv[0]);
        return 1;
    }

    read_file_hex(argv[1]);

    return 0;
}