#include <cmp.h>
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
  // 1. Depth bomb: 500 array starts (0x91 = array of size 1)
  char bomb[500];
  memset(bomb, 0x91, sizeof(bomb));
  write_raw(PROJECT_ROOT "/test/vectors/attack/mallicious/depth_bomb.bin", bomb, sizeof(bomb));

  // 2. OOM Map: Map32 tag (0xdf) requesting 0xFFFFFFFF elements.
  // A naive parser might try to allocate 4 Billion * sizeof(mp_object_kv_t) = 64GB of RAM instantly.
  char oom_map[] = {(char)0xdf, (char)0xff, (char)0xff, (char)0xff, (char)0xff};
  write_raw(PROJECT_ROOT "/test/vectors/attack/mallicious/oom_map.bin", oom_map, sizeof(oom_map));

  // 3. OOM String: Str32 tag (0xdb) requesting 0xFFFFFFFF bytes, followed by nothing.
  char oom_str[] = {(char)0xdb, (char)0xff, (char)0xff, (char)0xff, (char)0xff};
  write_raw(PROJECT_ROOT "/test/vectors/attack/mallicious/oom_str.bin", oom_str, sizeof(oom_str));

  // 4. OOM Array: Array32 tag (0xdd) requesting 0xFFFFFFFF elements.
  char oom_array[] = {(char)0xdd, (char)0xff, (char)0xff, (char)0xff, (char)0xff};
  write_raw(PROJECT_ROOT "/test/vectors/attack/mallicious/oom_array.bin", oom_array, sizeof(oom_array));

  // 5. OOM Bin: Bin32 tag (0xc6) requesting 0xFFFFFFFF bytes.
  char oom_bin[] = {(char)0xc6, (char)0xff, (char)0xff, (char)0xff, (char)0xff};
  write_raw(PROJECT_ROOT "/test/vectors/attack/mallicious/oom_bin.bin", oom_bin, sizeof(oom_bin));

  // 6. OOM Ext: Ext32 tag (0xc9) requesting 0xFFFFFFFF bytes.
  char oom_ext[] = {(char)0xc9, (char)0xff, (char)0xff, (char)0xff, (char)0xff, (char)0x01};
  write_raw(PROJECT_ROOT "/test/vectors/attack/mallicious/oom_ext.bin", oom_ext, sizeof(oom_ext));

  // 7. Map Depth Bomb: 500 levels of nested maps.
  // 0x81 (Map 1) -> 0x00 (Key 0) -> 0x81 (Map 1)...
  char map_bomb[1000];
  for (int i = 0; i < 500; i++) {
    map_bomb[i * 2] = (char)0x81;
    map_bomb[i * 2 + 1] = (char)0x00;
  }
  // Terminate inner most with a 0 value
  map_bomb[999] = (char)0x00;
  write_raw(PROJECT_ROOT "/test/vectors/attack/mallicious/depth_bomb_map.bin", map_bomb, sizeof(map_bomb));
}

void gen_mallformed() {
  // 1. Truncated map: 0x8A (map of size 10) but no elements.
  char trunc_map[] = {(char)0x8a};
  write_raw(PROJECT_ROOT "/test/vectors/attack/mallformed/truncated.bin", trunc_map, sizeof(trunc_map));

  // 2. Invalid Tag: 0xC1 is explicitly "Never Used" by the MessagePack specification.
  char invalid_tag[] = {(char)0xc1};
  write_raw(PROJECT_ROOT "/test/vectors/attack/mallformed/invalid_tag.bin", invalid_tag, sizeof(invalid_tag));

  // 3. Truncated UInt32: 0xce expects 4 bytes, but only 2 are provided.
  char trunc_uint32[] = {(char)0xce, (char)0x12, (char)0x34};
  write_raw(PROJECT_ROOT "/test/vectors/attack/mallformed/trunc_uint32.bin", trunc_uint32, sizeof(trunc_uint32));

  // 4. Truncated String: 0xd9 (Str8) length 255, but only 4 bytes of actual string data follow.
  char trunc_str[] = {(char)0xd9, (char)0xff, 'A', 'B', 'C', 'D'};
  write_raw(PROJECT_ROOT "/test/vectors/attack/mallformed/trunc_str.bin", trunc_str, sizeof(trunc_str));

  // 5. Truncated Extension: 0xd4 (Fixext 1) expects 1 byte of type and 1 byte of data. Missing data.
  char trunc_ext[] = {(char)0xd4, (char)0x01}; // Type 1, data missing
  write_raw(PROJECT_ROOT "/test/vectors/attack/mallformed/trunc_ext.bin", trunc_ext, sizeof(trunc_ext));

  // 6. Truncated Array Elements: Array of size 5, but only 3 elements present.
  char trunc_array_elements[] = {(char)0x95, 0x01, 0x02, 0x03}; 
  write_raw(PROJECT_ROOT "/test/vectors/attack/mallformed/trunc_array_elements.bin", trunc_array_elements, sizeof(trunc_array_elements));

  // 7. Truncated Map Pairs: Map of size 2, but only 3 elements (key, value, key, EOF).
  char trunc_map_pairs[] = {(char)0x82, (char)0xa4, 'k', 'e', 'y', '1', 0x01, (char)0xa4, 'k', 'e', 'y', '2'};
  write_raw(PROJECT_ROOT "/test/vectors/attack/mallformed/trunc_map_pairs.bin", trunc_map_pairs, sizeof(trunc_map_pairs));

  // 8. Truncated Float32: 0xca followed by 2 bytes instead of 4.
  char trunc_float32[] = {(char)0xca, 0x40, 0x49};
  write_raw(PROJECT_ROOT "/test/vectors/attack/mallformed/trunc_float32.bin", trunc_float32, sizeof(trunc_float32));

  // 9. Truncated Float64: 0xcb followed by 5 bytes instead of 8.
  char trunc_float64[] = {(char)0xcb, 0x40, 0x09, 0x21, 0xfb, 0x54};
  write_raw(PROJECT_ROOT "/test/vectors/attack/mallformed/trunc_float64.bin", trunc_float64, sizeof(trunc_float64));

  // 10. Truncated Bin8: 0xc4, len 10, but only 4 bytes follow.
  char trunc_bin8[] = {(char)0xc4, 0x0a, 0x01, 0x02, 0x03, 0x04};
  write_raw(PROJECT_ROOT "/test/vectors/attack/mallformed/trunc_bin8.bin", trunc_bin8, sizeof(trunc_bin8));

  // 11. Truncated Str16 Length: 0xda followed by 1 byte instead of 2.
  char trunc_str16_len[] = {(char)0xda, 0xff};
  write_raw(PROJECT_ROOT "/test/vectors/attack/mallformed/trunc_str16_len.bin", trunc_str16_len, sizeof(trunc_str16_len));

  // 12. Truncated Array32 Length: 0xdd followed by 3 bytes instead of 4.
  char trunc_array32_len[] = {(char)0xdd, 0x00, 0xff, 0xff};
  write_raw(PROJECT_ROOT "/test/vectors/attack/mallformed/trunc_array32_len.bin", trunc_array32_len, sizeof(trunc_array32_len));

  // 13. Ext8 Missing Type: 0xc7, len 5, EOF before type byte.
  char trunc_ext8_type[] = {(char)0xc7, 0x05};
  write_raw(PROJECT_ROOT "/test/vectors/attack/mallformed/trunc_ext8_type.bin", trunc_ext8_type, sizeof(trunc_ext8_type));

  // 14. FixExt8 Truncated Data: 0xd7, type 1, but only 4 bytes of data instead of 8.
  char trunc_fixext8_data[] = {(char)0xd7, 0x01, 0x0a, 0x0b, 0x0c, 0x0d};
  write_raw(PROJECT_ROOT "/test/vectors/attack/mallformed/trunc_fixext8_data.bin", trunc_fixext8_data, sizeof(trunc_fixext8_data));

  // 15. Deep Array Truncated: 10 levels of array, but no inner element.
  char trunc_deep_array[] = {(char)0x91, (char)0x91, (char)0x91, (char)0x91, (char)0x91, (char)0x91, (char)0x91, (char)0x91, (char)0x91, (char)0x91};
  write_raw(PROJECT_ROOT "/test/vectors/attack/mallformed/trunc_deep_array.bin", trunc_deep_array, sizeof(trunc_deep_array));
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