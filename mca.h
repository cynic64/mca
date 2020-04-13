#include <stdio.h>
#include <stdlib.h>
#include <zlib.h>
#include <strings.h>

// for zlib
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

int chunk_read_raw(unsigned char *buf, FILE *fp, int offset, unsigned long alloc) {
    // Reads the compressed chunk data at <offset>, discarding the chunk's
    // header and making certain that zlib compression is being used. Fails if
    // <alloc> would be exceeded.
    // Returns the number of bytes read on success, -1 on failure.

    fseek(fp, offset, SEEK_SET);

    char bytes[4];
    int count = fread(bytes, 1, 4, fp);

    if (count != 4) {
        return -1;
    }

    int size = bytes[0] * 0x1000000 + bytes[1] * 0x10000 + bytes[2] * 0x100 + bytes[3];

    if (size >= alloc) {
        return -1;
    }

    count = fread(bytes, 1, 1, fp);
    if (count != 1) {
        return -1;
    }

    if (bytes[0] != 2) {
        // chunk is not using zlib compression, we can't deal with it
        return -1;
    }

    // read chunk data
    fread(buf, 1, size, fp);

    return size;
}

int chunk_decompress(unsigned char *decomp, unsigned char *comp, unsigned long comp_len, unsigned long alloc) {
    // Reads <comp_len> bytes of data from <comp>, decompressing them into
    // <decomp>. If more than <alloc> bytes are needed for the decompressed
    // data, fails to avoid overstepping <decomp>.

    // Returns the decompressed byte count on success, -1 on failure.

    z_stream d_stream;

    d_stream.zalloc = zalloc;
    d_stream.zfree = zfree;
    d_stream.opaque = (voidpf) 0;

    d_stream.next_in = comp;
    d_stream.avail_in = 0;
    d_stream.next_out = decomp;

    int err = inflateInit(&d_stream);
    if (err != Z_OK) {
        return -1;
    }

    while (d_stream.total_in < comp_len) {
        d_stream.avail_in = d_stream.avail_out = 1;
        err = inflate(&d_stream, Z_NO_FLUSH);
        if (err == Z_STREAM_END) break;
        if (err != Z_OK) {
            inflateEnd(&d_stream);
            return -1;
        }
        if (d_stream.total_out >= alloc) {
            inflateEnd(&d_stream);
            return -1;
        }
    }

    inflateEnd(&d_stream);

    return d_stream.total_out;
}

int chunk_read(unsigned char *buf, FILE *fp, int offset, unsigned long alloc) {
    // Reads the chunk at <offset> at decompresses it into <buf>. If more than
    // <alloc> bytes are needed for the decompressed data, fails to avoid
    // overstepping <buf>

    // Returns the byte count on success and -1 on failure.

    // raw chunk data is always smaller than 1MB
    unsigned char *raw_buf = malloc(1048576);
    int count = chunk_read_raw(raw_buf, fp, offset, 1048576);
    if (count < 0) {
        return -1;
    }

    return chunk_decompress(buf, raw_buf, count, alloc);
}

int get_chunk_offset(FILE *fp, int x, int z) {
    // returns how many bytes into the file the chunk at (x, z) begins.
    // returns -1 if the chunk does not exist, -2 for error

    int header_offset = 4 * (z * 32 + x);
    fseek(fp, header_offset, SEEK_SET);

    // read bytes indicating where in the file the chunk begins
    unsigned char bytes[4];
    size_t bytes_read = fread(bytes, 1, 4, fp);

    if (bytes_read != 4) {
        // could not read bytes from file for whatever reason
        return -2;
    }

    if (bytes[0] + bytes[1] + bytes[2] == 0) {
        return -1;
    }

    return 4096 * (bytes[0] * 0x10000 + bytes[1] * 0x100 + bytes[2]);
}
