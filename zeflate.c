#include <stdio.h>
#include <stdlib.h>
#include <zlib.h>
#include "../helpers.h"

void *myalloc OF((void *, unsigned, unsigned));
void myfree OF((void *, void *));

void *myalloc(q, n, m)
    void *q;
    unsigned n, m;
{
    (void)q;
    return calloc(n, m);
}

void myfree(void *q, void *p)
{
    (void)q;
    free(p);
}

static alloc_func zalloc = myalloc;
static free_func zfree = myfree;

int main(int argc, char **argv) {
    if (argc < 2) {
        printf("Too few arguments (%i). Need at least <path>\n", argc);
        exit(1);
    }

    char *path = argv[1];

    FILE *fp = fopen(path, "rb");
    if (fp == NULL) {
        printf("Could not open %s\n", path);
        exit(1);
    }

    fseek(fp, 0, SEEK_END);
    long size = ftell(fp);
    printf("%li bytes in file\n", size);

    unsigned char *read_buf = malloc(size);
    fseek(fp, 0, SEEK_SET);
    int count = fread(read_buf, 1, size, fp);
    if (count != size) {
        printf("Could not read from file\n");
        exit(1);
    }

    dump(read_buf, size);

    // uncompress
    size_t out_size = size * 20;
    unsigned char * write_buf = malloc(out_size);

    z_stream d_stream;

    d_stream.zalloc = zalloc;
    d_stream.zfree = zfree;
    d_stream.opaque = (voidpf) 0;

    d_stream.next_in = read_buf;
    d_stream.avail_in = 0;
    d_stream.next_out = write_buf;

    int err = inflateInit(&d_stream);
    if (err != Z_OK) {
        printf("Error during inflateInit. Code: %i\n", err);
    }

    while (d_stream.total_out < out_size && d_stream.total_in < size) {
        d_stream.avail_in = d_stream.avail_out = 1;
        err = inflate(&d_stream, Z_NO_FLUSH);
        if (err == Z_STREAM_END) break;
        if (err != Z_OK) {
            printf("Error during inflate. Code: %i\n", err);
        }
    }

    printf("Uncompressed: %lu bytes\n", d_stream.total_out);
    dump(write_buf, d_stream.total_out);

    return 0;
}
