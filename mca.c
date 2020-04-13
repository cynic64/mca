/* Command-line program to interact with .mca files */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "mca.h"
#include "view.h"
#include "common.h"

void get_chunkmap(unsigned char chunkmap[32][32], FILE *fp);
void cmd_print(int arc, char **argv);
void cmd_read(int argc, char **argv);

int main(int argc, char **argv) {
    if (argc < 2) {
        printf("Too few arguments (%i)\n", argc - 1);
        printf("Need: <command> [args]\n");
        exit(1);
    }

    char *cmd = argv[1];
    if (strcmp(cmd, "print") == 0) {
        cmd_print(argc, argv);
    } else if (strcmp(cmd, "read") == 0) {
        cmd_read(argc, argv);
    } else {
        printf("Invalid command: %s\n", argv[2]);
        exit(1);
    }

    return 0;
}

void cmd_read(int argc, char **argv) {
    if (argc < 5) {
        printf("Too few arguments (%d): read <inpath> <x> <z> [path]\n", argc);
        exit(1);
    }

    char *path = argv[2];
    printf("Reading from: %s\n", path);

    FILE *fp = failing_open(path, "rb");

    int x = atoi(argv[3]);
    int z = atoi(argv[4]);

    printf("Reading chunk at (%d, %d)\n", x, z);

    int offset = get_chunk_offset(fp, x, z);

    if (offset < 0) {
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

void cmd_print(int argc, char **argv) {
    if (argc < 3) {
        printf("Too few arguments (%i). print <path>\n", argc);
        exit(1);
    }

    char *path = argv[2];
    printf("Opening: %s\n", path);
    FILE *fp = failing_open(path, "rb");

    unsigned char chunkmap[32][32];
    get_chunkmap(chunkmap, fp);
    print_chunkmap(chunkmap);

    fclose(fp);
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
