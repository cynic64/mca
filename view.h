#include <stdio.h>

void print_chunkmap(unsigned char chunkmap[32][32]) {
    for (int z = 0; z < 32; z++) {
        for (int x = 0; x < 32; x++) {
            if (chunkmap[z][x]) {
                printf("# ");
            } else {
                printf(". ");
            }
        }
        printf("\n");
    }
}

void dump(void *buf, int len) {
    unsigned char *bytes = (unsigned char *) buf;

    int i;
    for (i = 0; i < len; i++) {
        if (i % 16 == 0) {
            printf("%04x | ", i);
        }

        if (*bytes == 0) {
            printf(".. ");
        } else {
            printf("%02x ", *bytes);
        }

        if (i % 16 == 15) {
            // print the last 16 bytes in ASCII, and then a newline
            printf(" | ");
            for (int j = 0; j < 16; j++) {
                char c = *(bytes-15+j);
                if (c >= 32 && c != 127) {
                    putchar(c);
                } else {
                    putchar('.');
                }
            }

            printf("\n");
        }

        bytes++;
    }

    printf("\n");
}
