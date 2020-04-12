/* Command-line program to interact with .mca files */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "mca.h"
#include "view.h"

void get_chunkmap(unsigned char chunkmap[32][32], FILE *fp);
void cmd_print(FILE *fp);
void cmd_read(int argc, char **argv, FILE *fp);

int main(int argc, char **argv) {
    if (argc < 3) {
        printf("Too few arguments (%i)\n", argc - 1);
        printf("Need: <path> <command> [args]\n");
        exit(1);
    }

    char *path = argv[1];

    FILE *fp = fopen(path, "r");

    if (fp == NULL) {
        printf("Could not open file %s!\n", path);
        exit(1);
    }

    if (strcmp(argv[2], "print") == 0) {
        cmd_print(fp);
    } else if (strcmp(argv[2], "read") == 0) {
        cmd_read(argc, argv, fp);
    } else {
        printf("Invalid command: %s\n", argv[2]);
        exit(1);
    }

    // cleanup
    fclose(fp);

    return 0;
}

void cmd_read(int argc, char **argv, FILE *fp) {
    if (argc < 5) {
        printf("Too few arguments (%d): 'read' requires x, z, [path]\n", argc);
        exit(1);
    }

    int x = atoi(argv[3]);
    int z = atoi(argv[4]);

    printf("Reading chunk at (%d, %d)\n", x, z);

    int offset = get_chunk_offset(fp, x, z);

    if (offset  < 0) {
        printf("Chunk at (%d, %d) cannot be read because it does not exist.\n", x, z);
        exit(1);
    }

    // allocate a megabyte for the chunk
    unsigned char *buf = malloc(1048576);
    int count = chunk_read(buf, fp, offset, 1048576);
    if (count < 0) {
        printf("Error reading chunk.\n");
        exit(1);
    }

    printf("Read %i bytes from chunk (offset %#x)\n", count, offset);

    int nbt_offset = chunk_nbt_offset(buf, "Palette", count);
    printf("NBT Offset: %#x\n", nbt_offset);
    int nbt_size = chunk_parse_nbt(buf + nbt_offset, count - nbt_offset);
    printf("Size of NBT entry at %#x: %i\n", nbt_offset, nbt_size);

    // write to file if given a path, otherwise hex-dump to stdout
    if (argc == 6) {
        char *path = argv[5];
        FILE *wp = fopen(path, "w");
        if (wp == NULL) {
            printf("Error opening %s for writing\n", path);
            exit(1);
        }

        fwrite(buf, 1, count, wp);

        fclose(wp);
    } else {
        dump(buf, count);
    }

    free(buf);
}

void cmd_print(FILE *fp) {
    unsigned char chunkmap[32][32];
    get_chunkmap(chunkmap, fp);
    print_chunkmap(chunkmap);
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
