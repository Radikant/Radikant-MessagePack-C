#include "../../reference/cmp.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Helper to use FILE* as cmp writer
static size_t file_writer(cmp_ctx_t *ctx, const void *data, size_t count) {
  return fwrite(data, 1, count, (FILE *)ctx->buf);
}

void write_valid(const char *path, void (*gen_fn)(cmp_ctx_t *)) {
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

void gen_simple(cmp_ctx_t *cmp) {
  cmp_write_array(cmp, 19);

  cmp_write_nil(cmp);
  cmp_write_true(cmp);
  cmp_write_false(cmp);

  // Unsigned explicitly forced types
  cmp_write_uinteger(cmp,
                     127); // Positive FixInt (implicitly selected by value)
  cmp_write_u8(cmp, 255);
  cmp_write_u16(cmp, 65535);
  cmp_write_u32(cmp, 4294967295ULL);
  cmp_write_u64(cmp, 18446744073709551615ULL);

  // Signed explicitly forced types
  cmp_write_integer(cmp, -32); // Negative FixInt (implicitly selected by value)
  cmp_write_s8(cmp, -128);
  cmp_write_s16(cmp, -32768);
  cmp_write_s32(cmp, -2147483648LL);
  cmp_write_s64(cmp, -9223372036854775807LL - 1LL);

  cmp_write_float(cmp, 3.14159f);
  cmp_write_double(cmp, 2.718281828459);

  cmp_write_str(cmp, "hello", 5);

  char bin[] = {0x01, 0x02, 0x03};
  cmp_write_bin(cmp, bin, 3);
  cmp_write_map(cmp, 1);
  cmp_write_str(cmp, "key", 3);
  cmp_write_str(cmp, "value", 5);

  char ext[] = {0x42, 0x43};
  cmp_write_ext(cmp, 1, 2, ext);
}

void gen_hard(cmp_ctx_t *cmp) {
  // Let's create an array of 10,000 mixed elements.
  cmp_write_array(cmp, 10000);

  for (int i = 0; i < 10000; i++) {
    int type_selector = i % 10;
    
    switch (type_selector) {
      case 0:
        cmp_write_integer(cmp, i * 12345);
        break;
      case 1:
        cmp_write_bool(cmp, i % 2 == 0);
        break;
      case 2:
        cmp_write_float(cmp, (float)i * 1.11f);
        break;
      case 3:
        cmp_write_double(cmp, (double)i * -2.22);
        break;
      case 4:
        cmp_write_str(cmp, "tiny_string", 11);
        break;
      case 5: {
        char bin_data[8] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07};
        cmp_write_bin(cmp, bin_data, 8);
        break;
      }
      case 6: {
        char ext_data[4] = {0xAA, 0xBB, 0xCC, 0xDD};
        cmp_write_ext(cmp, 10, 4, ext_data);
        break;
      }
      case 7:
        cmp_write_nil(cmp);
        break;
      case 8:
        // Small inner array
        cmp_write_array(cmp, 2);
        cmp_write_integer(cmp, i);
        cmp_write_str(cmp, "arr_str", 7);
        break;
      case 9:
        // Small inner map
        cmp_write_map(cmp, 1);
        cmp_write_str(cmp, "map_key", 7);
        cmp_write_double(cmp, 3.14159);
        break;
    }
  }
}

void gen_difficult(cmp_ctx_t *cmp) {
  // Top level array of 4 complex elements
  cmp_write_array(cmp, 4);

  // Element 1: Deeply nested Maps and Arrays
  cmp_write_map(cmp, 3);
  
  // Key 1: integer -> Value 1: Map
  cmp_write_integer(cmp, -999);
  cmp_write_map(cmp, 1);
  cmp_write_str(cmp, "deep_key", 8);
  cmp_write_array(cmp, 5); // 5 elements inside
  cmp_write_nil(cmp);
  cmp_write_true(cmp);
  cmp_write_float(cmp, -1.2345f);
  
  char ext_data1[] = "EXT";
  cmp_write_ext(cmp, 42, 3, ext_data1);
  
  char bin_data1[] = "BLOB";
  cmp_write_bin(cmp, bin_data1, 4);

  // Key 2: string -> Value 2: Array of Maps
  cmp_write_str(cmp, "array_of_maps", 13);
  cmp_write_array(cmp, 2);
  cmp_write_map(cmp, 1);
  cmp_write_false(cmp);
  cmp_write_integer(cmp, 100);
  cmp_write_map(cmp, 1);
  cmp_write_str(cmp, "xyz", 3);
  cmp_write_double(cmp, 3.1415926535);

  // Key 3: Map -> Value 3: Array (Testing complex keys)
  cmp_write_map(cmp, 1);
  cmp_write_integer(cmp, 42);
  cmp_write_str(cmp, "universe", 8);
  cmp_write_array(cmp, 0);

  // Element 2: Giant alternating Array
  cmp_write_array(cmp, 100);
  for (int i = 0; i < 100; i++) {
    if (i % 4 == 0) cmp_write_integer(cmp, i);
    else if (i % 4 == 1) cmp_write_str(cmp, "x", 1);
    else if (i % 4 == 2) cmp_write_map(cmp, 0);
    else cmp_write_array(cmp, 0);
  }

  // Element 3: Ext nested in a map
  cmp_write_map(cmp, 1);
  char ext_z[] = "Z";
  cmp_write_ext(cmp, 99, 1, ext_z);
  char ext_zz[] = "ZZ";
  cmp_write_ext(cmp, 100, 2, ext_zz);

  // Element 4: Binary key with a Nil value
  cmp_write_map(cmp, 1);
  char bin_data[256];
  for (int i = 0; i < 256; i++) bin_data[i] = (char)i;
  cmp_write_bin(cmp, bin_data, 256);
  cmp_write_nil(cmp);
}

void write_raw(const char *path, const char *data, size_t len) {
  FILE *f = fopen(path, "wb");
  if (f) {
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
  write_raw(PROJECT_ROOT "/test/vectors/attack/mallicious/depth_bomb.bin", bomb,
            sizeof(bomb));
}

void gen_mallformed() {
  // Truncated map: 0x8A (map of size 10) but no elements.
  char trunc[] = {(char)0x8a};
  write_raw(PROJECT_ROOT "/test/vectors/attack/mallformed/truncated.bin", trunc,
            1);

  // Invalid tag: 0xc1 (never used)
  char inv[] = {(char)0xc1};
  write_raw(PROJECT_ROOT "/test/vectors/attack/mallformed/invalid_tag.bin", inv,
            1);
}

int main(void) {
  write_valid(PROJECT_ROOT "/test/vectors/test/simple/simple1.bin", gen_simple);
  write_valid(PROJECT_ROOT "/test/vectors/test/hard/hard1.bin", gen_hard);
  write_valid(PROJECT_ROOT "/test/vectors/test/difficult/diff1.bin",
              gen_difficult);

  gen_mallicious();
  gen_mallformed();

  printf("Test vectors generated successfully!\n");
  return 0;
}