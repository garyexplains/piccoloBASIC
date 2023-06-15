#include <stdio.h>

/*
 * Compile with:
 * gcc -o upload2pb upload2pb.c
*/

int main(int argc, char *argv[]) {
    FILE *fp;
    char ch;

    if (argc != 3) {
        printf("Usage: %s <filename> <device>\n");
        printf("eg: %s main.bas /dev/ttyACM0\n");
        return 1;
    }

    fp = fopen(argv[1], "r");

    while ((ch = fgetc(fp)) != EOF) {
        printf("%c", ch);
    }

    fclose(fp);

    return 0;
}