#include<stdio.h>

void dump(void *addr, int len) {
    unsigned char *bytes = (unsigned char *) addr;

    for (int i = 0; i < len; i++) {
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
                if (c >= 32) {
                    printf("%c ", c);
                } else {
                    printf(". ");
                }
            }

            printf("\n");
        }

        bytes++;
    }

    printf("\n");
}

