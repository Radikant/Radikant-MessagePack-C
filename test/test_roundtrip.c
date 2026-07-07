#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../SRC/messagepack.h"

// Recursive AST Equality Checker
static bool ast_equal(mp_object_t *a, mp_object_t *b) {
  if (a->type != b->type) return false;

  switch (a->type) {
  case MP_TYPE_NIL:
    return true;
  case MP_TYPE_BOOLEAN:
    return a->via.boolean == b->via.boolean;
  case MP_TYPE_POSITIVE_INTEGER:
    return a->via.u64 == b->via.u64;
  case MP_TYPE_NEGATIVE_INTEGER:
    return a->via.i64 == b->via.i64;
  case MP_TYPE_FLOAT32:
    return a->via.f32 == b->via.f32;
  case MP_TYPE_FLOAT64:
    return a->via.f64 == b->via.f64;
  case MP_TYPE_STR:
    if (a->via.str.size != b->via.str.size) return false;
    return memcmp(a->via.str.ptr, b->via.str.ptr, a->via.str.size) == 0;
  case MP_TYPE_BIN:
    if (a->via.bin.size != b->via.bin.size) return false;
    return memcmp(a->via.bin.ptr, b->via.bin.ptr, a->via.bin.size) == 0;
  case MP_TYPE_EXT:
    if (a->via.ext.type != b->via.ext.type) return false;
    if (a->via.ext.size != b->via.ext.size) return false;
    return memcmp(a->via.ext.ptr, b->via.ext.ptr, a->via.ext.size) == 0;
  case MP_TYPE_ARRAY:
    if (a->via.array.size != b->via.array.size) return false;
    for (uint32_t i = 0; i < a->via.array.size; i++) {
      if (!ast_equal(&a->via.array.ptr[i], &b->via.array.ptr[i])) return false;
    }
    return true;
  case MP_TYPE_MAP:
    if (a->via.map.size != b->via.map.size) return false;
    for (uint32_t i = 0; i < a->via.map.size; i++) {
      if (!ast_equal(a->via.map.ptr[i].key, b->via.map.ptr[i].key)) return false;
      if (!ast_equal(a->via.map.ptr[i].val, b->via.map.ptr[i].val)) return false;
    }
    return true;
  }
  return false;
}

#ifndef PROJECT_ROOT
#define PROJECT_ROOT "."
#endif

static char *read_file_to_mem(const char *path, size_t *out_size) {
  FILE *f = fopen(path, "rb");
  if (!f) return NULL;
  fseek(f, 0, SEEK_END);
  *out_size = ftell(f);
  fseek(f, 0, SEEK_SET);
  char *buf = (char *)malloc(*out_size);
  if (buf) fread(buf, 1, *out_size, f);
  fclose(f);
  return buf;
}

static void test_roundtrip(const char *name, const char *path) {
  printf("Testing Round-Trip Fidelity: %s\n", name);
  size_t original_size;
  char *original_buf = read_file_to_mem(path, &original_size);
  assert(original_buf != NULL);

  mp_zone_t zone1, zone2;
  mp_zone_init(&zone1, 4096);
  mp_zone_init(&zone2, 4096);

  mp_object_t ast1;
  mp_error_t err = mp_parse_memory(&zone1, original_buf, original_size, &ast1);
  assert(err == MP_OK);

  // Re-encode it
  // Allocate a large enough buffer for re-encoding
  char *encoded_buf = (char *)malloc(original_size + 1024);
  mp_memory_stream_ctx_t mem_ctx;
  mp_stream_t stream;
  mp_memory_stream_init_write_fixed(&stream, &mem_ctx, encoded_buf, original_size + 1024);
  
  err = mp_encode_object(&stream, &ast1);
  assert(err == MP_OK);

  // Decode it back
  mp_object_t ast2;
  err = mp_parse_memory(&zone2, encoded_buf, mem_ctx.size, &ast2);
  assert(err == MP_OK);

  // Check absolute structural equality
  bool equal = ast_equal(&ast1, &ast2);
  if (!equal) {
    printf("  [FAIL] AST structures do not match for %s!\n", name);
    exit(1);
  }
  printf("  [PASS] AST perfectly preserved.\n");

  mp_zone_destroy(&zone1);
  mp_zone_destroy(&zone2);
  free(original_buf);
  free(encoded_buf);
}

int main() {
  test_roundtrip("Simple Vector", PROJECT_ROOT "/test/vectors/test/simple/simple1.bin");
  test_roundtrip("Hard Vector", PROJECT_ROOT "/test/vectors/test/hard/hard1.bin");
  test_roundtrip("Difficult Vector", PROJECT_ROOT "/test/vectors/test/difficult/diff1.bin");
  
  printf("All round-trip fidelity tests completed successfully.\n");
  return 0;
}
