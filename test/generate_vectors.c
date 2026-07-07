#include "../reference/cmp.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Helper to use FILE* as cmp writer
static size_t file_writer(cmp_ctx_t *ctx, const void *data, size_t count) {
    return fwrite(data, 1, count, (FILE *)ctx->buf);
}

void write_valid(const char* path, void (*gen_fn)(cmp_ctx_t*)) {
    FILE *f = fopen(path, "wb");
    if (!f) {
        printf("Failed to open %s for writing\n", path);
        return;
    }
    cmp_ctx_t cmp;
    cmp_init(&cmp, f, NULL, NULL, file_writer);
    gen_fn(&cmp);
    fclose(f);
}

void gen_simple(cmp_ctx_t* cmp) {
    cmp_write_array(cmp, 3);
    cmp_write_integer(cmp, 42);
    cmp_write_str(cmp, "hello", 5);
    cmp_write_bool(cmp, true);
}

void gen_hard(cmp_ctx_t* cmp) {
    // 32-bit float, huge string
    cmp_write_map(cmp, 2);
    cmp_write_str(cmp, "pi", 2);
    cmp_write_float(cmp, 3.14159f);
    cmp_write_str(cmp, "big_str", 7);
    
    char* big = (char*)malloc(60000);
    if(big) {
        memset(big, 'A', 60000);
        cmp_write_str(cmp, big, 60000);
        free(big);
    }
}

void gen_difficult(cmp_ctx_t* cmp) {
    cmp_write_map(cmp, 2);
    // int key
    cmp_write_integer(cmp, 1234);
    char bin_data[256];
    for (int i=0; i<256; i++) bin_data[i] = (char)i;
    cmp_write_bin(cmp, bin_data, 256);
    // str key
    cmp_write_str(cmp, "key2", 4);
    cmp_write_array(cmp, 2);
    cmp_write_nil(cmp);
    cmp_write_integer(cmp, -100000);
}

void write_raw(const char* path, const char* data, size_t len) {
    FILE* f = fopen(path, "wb");
    if(f) {
        fwrite(data, 1, len, f);
        fclose(f);
    } else {
        printf("Failed to open %s for writing\n", path);
    }
}

#ifndef PROJECT_ROOT
#define PROJECT_ROOT "."
#endif

void gen_mallicious() {
    // Depth bomb: 500 array starts (0x91 = array of size 1)
    char bomb[500];
    memset(bomb, 0x91, sizeof(bomb));
    write_raw(PROJECT_ROOT "/test/vectors/attack/mallicious/depth_bomb.bin", bomb, sizeof(bomb));
}

void gen_mallformed() {
    // Truncated map: 0x8A (map of size 10) but no elements.
    char trunc[] = { (char)0x8a };
    write_raw(PROJECT_ROOT "/test/vectors/attack/mallformed/truncated.bin", trunc, 1);
    
    // Invalid tag: 0xc1 (never used)
    char inv[] = { (char)0xc1 };
    write_raw(PROJECT_ROOT "/test/vectors/attack/mallformed/invalid_tag.bin", inv, 1);
}

int main(void) {
    write_valid(PROJECT_ROOT "/test/vectors/test/simple/simple1.bin", gen_simple);
    write_valid(PROJECT_ROOT "/test/vectors/test/hard/hard1.bin", gen_hard);
    write_valid(PROJECT_ROOT "/test/vectors/test/difficult/diff1.bin", gen_difficult);
    
    gen_mallicious();
    gen_mallformed();
    
    printf("Test vectors generated successfully!\n");
    return 0;
}
