#include<stdio.h>
#include<stdlib.h>

int read_chunk(unsigned char *buf, FILE *fp, int offset) {
    // returns the number of bytes read (minus the header) on success, or -1 on
    // failure

    fseek(fp, offset, SEEK_SET);

    char bytes[4];
    int count = fread(bytes, 1, 4, fp);

    if (count != 4) {
        return -1;
    }

    int size = bytes[0] * 0x1000000 + bytes[1] * 0x10000 + bytes[2] * 0x100 + bytes[3];

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
