/* Command-line program to interact with .mca files */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "helpers.h"
#include "mca.h"

void print_chunkmap(unsigned char chunkmap[32][32]);
void get_chunkmap(unsigned char chunkmap[32][32], FILE *fp);

int main(int argc, char **argv) {
    if (argc < 3) {
        printf("Too few arguments (%i)\n", argc - 1);
        printf("Need: <path> <command> [args]\n");
        exit(1);
    }

    char *path = argv[1];

    printf("Opening: %s\n", path);

    FILE *fp = fopen(path, "r");

    if (fp == NULL) {
        printf("Could not open file %s!\n", path);
        exit(1);
    }

    if (strcmp(argv[2], "print") == 0) {
        unsigned char chunkmap[32][32];
        get_chunkmap(chunkmap, fp);
        print_chunkmap(chunkmap);
    } else if (strcmp(argv[2], "read") == 0) {
        if (argc < 6) {
            printf("Too few arguments (%d): 'read' requires x, z, out\n", argc);
            exit(1);
        }

        int x = atoi(argv[3]);
        int z = atoi(argv[4]);

        char *path = argv[5];

        printf("Reading chunk at (%d, %d)\n", x, z);

        int chunk_offset = get_chunk_offset(fp, x, z);

        if (chunk_offset < 0) {
            printf("Chunk at (%d, %d) cannot be read because it does not exist.\n", x, z);
            exit(1);
        }

        // allocate a megabyte for the chunk (chunks are always <1MB)
        unsigned char *chunk_data = malloc(1048576);
        int bytes_read = read_chunk(chunk_data, fp, chunk_offset);
        printf("Read %i bytes from chunk (offset %#x)\n", bytes_read, chunk_offset);

        printf("Writing to %s\n", path);
        fclose(fp);
        fp = fopen(path, "wb");

        if (fp == NULL) {
            printf("Couldn't open %s\n", path);
            exit(1);
        }

        fwrite(chunk_data, 1, bytes_read, fp);
    } else {
        printf("Invalid command: %s\n", argv[2]);
        exit(1);
    }

    // cleanup
    fclose(fp);

    return 0;
}

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

void get_chunkmap(unsigned char chunkmap[32][32], FILE *fp) {
    for (int z = 0; z < 32; z++) {
        for (int x = 0; x < 32; x++) {
            if (get_chunk_offset(fp, x, z) >= 0) {
                chunkmap[z][x] = 1;
            } else {
                chunkmap[z][x] = 0;
            }
        }
    }
}
