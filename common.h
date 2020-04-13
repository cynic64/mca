#include <stdio.h>

FILE *failing_open(char *path, char *mode) {
    FILE *fp = fopen(path, mode);

    if (fp == NULL) {
        printf("Could not open %s\n", path);
        exit(1);
    }

    return fp;
}
