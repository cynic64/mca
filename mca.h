#include <stdio.h>
#include <stdlib.h>
#include <zlib.h>
#include <strings.h>

int chunk_parse_nbt(unsigned char *buf, unsigned long size);

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

struct NBT {
    char name[255];
    unsigned char *data;
    int data_len;
    struct NBT *children;
    int child_count;
};

int chunk_nbt_offset(unsigned char *buf, char *str, unsigned long len) {
    // str should to be null-terminated

    unsigned char *ptr = buf;
    while (ptr++ < buf + len - strlen(str)) {
        int matches = 1;
        for (int i = 0; i < strlen(str); i++) {
            if (str[i] != *(ptr + i)) {
                matches = 0;
                break;
            }
        }

        if (!matches) continue;

        // 3 bytes are used to encode the type and length of the name of the
        // tag
        return ptr - buf - 3;
    }

    return -1;
}

int nbt_parse_value(unsigned char *buf, unsigned char type) {
    unsigned char *ptr = buf;

    // Parses just the value of an NBT entry, returning its length.
    switch(type) {
        case 9 :
            printf("\tList\n");

            // first byte is list type
            unsigned char l_type = *ptr++;

            // next 4 bytes (big-endian) are list length
            unsigned l_len = *ptr++ * 0x1000000 + *ptr++ * 0x10000 + *ptr++ * 0x100 + *ptr++;

            printf("\tType: %i, length: %u\n", l_type, l_len);

            for(int i = 0; i < l_len; i++) {
                ptr += nbt_parse_value(ptr, l_type);
            }

            break;

        case 8 :
            printf("\tString\n");

            // first two bytes are big-endian string length
            int str_len = *ptr++ * 0x100 + *ptr++;

            char str[255];
            memcpy(str, ptr, str_len);
            str[str_len] = 0;
            printf("\tLength: %i, content: %s\n", str_len, str);

            ptr += str_len;

            break;

        case 10 :
            printf("\tCompound\n");

            // parse until we hit end-of-compound
            while (*ptr != 0) {
                ptr += chunk_parse_nbt(ptr, 1048576);
            }

            printf("\tEnd compound\n");

            ptr++;

            break;

        case 12 :
            printf("\tLong array\n");
            // first four bytes are array length
            unsigned la_len = *ptr++ * 0x1000000 + *ptr++ * 0x10000 + *ptr++ * 0x100 + *ptr++;
            printf("\tLength: %u\n", la_len);

            ptr += la_len * 8;

            break;

        case 3 :
            printf("\tInt\n");

            int i = *ptr++ * 0x1000000 + *ptr++ * 0x10000 + *ptr++ * 0x100 + *ptr++;
            printf("\tValue: %i\n", i);

            break;

        case 4 :
            printf("\tLong\n");

            long l = 0;
            long mul = 0x100000000000000;

            // doesn't handle negative numbers
            for (int j = 0; j < 8; j++) {
                l += *ptr++ * mul;
                mul >>= 8;
            }

            printf("\tValue: %li\n", l);

            break;

        case 11 :
            printf("\tInt array\n");

            // array length
            unsigned i_len = *ptr++ * 0x1000000 + *ptr++ * 0x10000 + *ptr++ * 0x100 + *ptr++;
            printf("\tLength: %u\n", i_len);

            ptr += i_len * 4;

            break;

        case 1 :
            printf("\tByte\n");
            ptr++;

            break;

        case 7:
            printf("\tByte array\n");
            // first four bytes are length
            unsigned b_len = *ptr++ * 0x1000000 + *ptr++ * 0x10000 + *ptr++ * 0x100 + *ptr++;
            printf("\tLength: %u\n", b_len);

            ptr += b_len;

            break;

        default :
            printf("Don't know type: %i\n", type);
            exit(1);
    }

    return ptr - buf;
}

int chunk_parse_nbt(unsigned char *buf, unsigned long size) {
    // Parses a single NBT entry and returns its length in bytes.
    // Fails if buf + size is exceeded, returning -1.

    unsigned char *ptr = buf;

    // first byte is type
    unsigned char type = *ptr++;
    printf("Tag type ID: %u (offset %#x)\n", type, ptr - buf - 1);

    if (type == 0) {
        printf("\tEnd Compound\n");
        return 1;
    }

    // next two bytes are big-endian name length
    int name_len = *ptr++ * 0x100 + *ptr++;
    printf("\tName len: %i\n", name_len);

    if (name_len == 0) {
        printf("Zero length name\n");
        exit(1);
    }

    char name[255];
    memcpy(name, ptr, name_len);
    // add a trailing \0
    name[name_len] = 0;
    printf("\tName: %s\n", name);

    ptr += name_len;

    ptr += nbt_parse_value(ptr, type);

    return ptr - buf;
}

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
