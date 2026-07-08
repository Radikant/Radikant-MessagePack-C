// Std
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Radikant
#include <mpack.h>
#include <radikant-messagepack-c.h>
#include <radikant-probe-c.h>

#include <cmp.h>

#ifndef PROJECT_ROOT
#define PROJECT_ROOT "."
#endif

// Helper to read entire file into memory
static char *read_file_to_mem(const char *path, size_t *out_size) {
  FILE *f = fopen(path, "rb");
  if (!f)
    return NULL;
  fseek(f, 0, SEEK_END);
  *out_size = ftell(f);
  fseek(f, 0, SEEK_SET);
  char *buf = (char *)malloc(*out_size);
  if (buf)
    fread(buf, 1, *out_size, f);
  fclose(f);
  return buf;
}

// cmp memory reader context
typedef struct {
  char *buf;
  size_t size;
  size_t offset;
} cmp_mem_t;

static bool cmp_mem_reader(cmp_ctx_t *ctx, void *data, size_t limit) {
  cmp_mem_t *mem = (cmp_mem_t *)ctx->buf;
  if (mem->offset + limit > mem->size)
    return false;
  memcpy(data, mem->buf + mem->offset, limit);
  mem->offset += limit;
  return true;
}

// Global buffers to avoid reading files during test iterations
char *buf_simple = NULL;
size_t size_simple = 0;
char *buf_hard = NULL;
size_t size_hard = 0;
char *buf_diff = NULL;
size_t size_diff = 0;

// Benchmark iterations
#define ITERS_SIMPLE 200000
#define ITERS_HARD 50000
#define ITERS_DIFF 20000

// --- Simple Vector ---

void comp_simple_cmp(perf_result_t *res) {
  clock_t start = clock();
  for (int i = 0; i < ITERS_SIMPLE; i++) {
    cmp_mem_t mem = {.buf = buf_simple, .size = size_simple, .offset = 0};
    cmp_ctx_t cmp;
    cmp_init(&cmp, &mem, cmp_mem_reader, NULL, NULL);
    cmp_skip_object_no_limit(&cmp);
    assert(cmp.error == 0);
    assert(mem.offset == size_hard || mem.offset == size_simple ||
           mem.offset == size_diff);
  }
  clock_t end = clock();
  double elapsed = (double)(end - start) / CLOCKS_PER_SEC;
  double mb = (double)(size_simple * ITERS_SIMPLE) / (1024.0 * 1024.0);
  perf_record_speed(res, mb / elapsed);
}

void comp_simple_radikant(perf_result_t *res) {
  clock_t start = clock();
  for (int i = 0; i < ITERS_SIMPLE; i++) {
    mp_zone_t zone;
    mp_zone_init(&zone, 4096);

    mp_stream_buffer_t mem_buffer;
    mp_stream_t stream;
    mp_stream_init_read(&stream, &mem_buffer, buf_simple, size_simple);

    mp_decoder_t decoder;
    mp_decoder_init(&decoder, &stream, &zone);
    decoder.zero_copy_strings = true;

    mp_object_t obj;
    mp_error_t err = mp_decode(&decoder, &obj);

    assert(err == MP_OK);
    assert(mem_buffer.offset == size_hard || mem_buffer.offset == size_simple ||
           mem_buffer.offset == size_diff);
    mp_zone_destroy(&zone);
  }
  clock_t end = clock();

  double elapsed = (double)(end - start) / CLOCKS_PER_SEC;
  double mb = (double)(size_simple * ITERS_SIMPLE) / (1024.0 * 1024.0);
  perf_record_speed(res, mb / elapsed);
}

// --- Hard Vector ---

void comp_hard_cmp(perf_result_t *res) {
  clock_t start = clock();
  for (int i = 0; i < ITERS_HARD; i++) {
    cmp_mem_t mem = {.buf = buf_hard, .size = size_hard, .offset = 0};
    cmp_ctx_t cmp;
    cmp_init(&cmp, &mem, cmp_mem_reader, NULL, NULL);
    cmp_skip_object_no_limit(&cmp);
    assert(cmp.error == 0);
    assert(mem.offset == size_hard || mem.offset == size_simple ||
           mem.offset == size_diff);
  }
  clock_t end = clock();
  double elapsed = (double)(end - start) / CLOCKS_PER_SEC;
  double mb = (double)(size_hard * ITERS_HARD) / (1024.0 * 1024.0);
  perf_record_speed(res, mb / elapsed);
}

void comp_hard_radikant(perf_result_t *res) {
  clock_t start = clock();
  for (int i = 0; i < ITERS_HARD; i++) {
    mp_zone_t zone;
    mp_zone_init(&zone, 131072);

    mp_stream_buffer_t mem_buffer;
    mp_stream_t stream;
    mp_stream_init_read(&stream, &mem_buffer, buf_hard, size_hard);

    mp_decoder_t decoder;
    mp_decoder_init(&decoder, &stream, &zone);
    decoder.zero_copy_strings = true;

    mp_object_t obj;
    mp_error_t err = mp_decode(&decoder, &obj);

    assert(err == MP_OK);
    assert(mem_buffer.offset == size_hard || mem_buffer.offset == size_simple ||
           mem_buffer.offset == size_diff);
    mp_zone_destroy(&zone);
  }
  clock_t end = clock();

  double elapsed = (double)(end - start) / CLOCKS_PER_SEC;
  double mb = (double)(size_hard * ITERS_HARD) / (1024.0 * 1024.0);
  perf_record_speed(res, mb / elapsed);
}

// --- Difficult Vector ---

void comp_diff_cmp(perf_result_t *res) {
  clock_t start = clock();
  for (int i = 0; i < ITERS_DIFF; i++) {
    cmp_mem_t mem = {.buf = buf_diff, .size = size_diff, .offset = 0};
    cmp_ctx_t cmp;
    cmp_init(&cmp, &mem, cmp_mem_reader, NULL, NULL);
    cmp_skip_object_no_limit(&cmp);
    assert(cmp.error == 0);
    assert(mem.offset == size_hard || mem.offset == size_simple ||
           mem.offset == size_diff);
  }
  clock_t end = clock();
  double elapsed = (double)(end - start) / CLOCKS_PER_SEC;
  double mb = (double)(size_diff * ITERS_DIFF) / (1024.0 * 1024.0);
  perf_record_speed(res, mb / elapsed);
}

void comp_diff_radikant(perf_result_t *res) {
  clock_t start = clock();
  for (int i = 0; i < ITERS_DIFF; i++) {
    mp_zone_t zone;
    mp_zone_init(&zone, 32768);

    mp_stream_buffer_t mem_buffer;
    mp_stream_t stream;
    mp_stream_init_read(&stream, &mem_buffer, buf_diff, size_diff);

    mp_decoder_t decoder;
    mp_decoder_init(&decoder, &stream, &zone);
    decoder.zero_copy_strings = true;

    mp_object_t obj;
    mp_error_t err = mp_decode(&decoder, &obj);

    assert(err == MP_OK);
    assert(mem_buffer.offset == size_hard || mem_buffer.offset == size_simple ||
           mem_buffer.offset == size_diff);
    mp_zone_destroy(&zone);
  }
  clock_t end = clock();

  double elapsed = (double)(end - start) / CLOCKS_PER_SEC;
  double mb = (double)(size_diff * ITERS_DIFF) / (1024.0 * 1024.0);
  perf_record_speed(res, mb / elapsed);
}

void comp_simple_mpack(perf_result_t *res) {
  clock_t start = clock();
  for (int i = 0; i < ITERS_SIMPLE; i++) {
    mpack_tree_t tree;
    mpack_tree_init_data(&tree, buf_simple, size_simple);
    mpack_tree_parse(&tree);
    mpack_error_t err = mpack_tree_error(&tree);
    assert(err == mpack_ok);
    mpack_tree_destroy(&tree);
  }
  clock_t end = clock();
  double elapsed = (double)(end - start) / CLOCKS_PER_SEC;
  double mb = (double)(size_simple * ITERS_SIMPLE) / (1024.0 * 1024.0);
  perf_record_speed(res, mb / elapsed);
}

void comp_hard_mpack(perf_result_t *res) {
  clock_t start = clock();
  for (int i = 0; i < ITERS_HARD; i++) {
    mpack_tree_t tree;
    mpack_tree_init_data(&tree, buf_hard, size_hard);
    mpack_tree_parse(&tree);
    mpack_error_t err = mpack_tree_error(&tree);
    assert(err == mpack_ok);
    mpack_tree_destroy(&tree);
  }
  clock_t end = clock();
  double elapsed = (double)(end - start) / CLOCKS_PER_SEC;
  double mb = (double)(size_hard * ITERS_HARD) / (1024.0 * 1024.0);
  perf_record_speed(res, mb / elapsed);
}

void comp_diff_mpack(perf_result_t *res) {
  clock_t start = clock();
  for (int i = 0; i < ITERS_DIFF; i++) {
    mpack_tree_t tree;
    mpack_tree_init_data(&tree, buf_diff, size_diff);
    mpack_tree_parse(&tree);
    mpack_error_t err = mpack_tree_error(&tree);
    assert(err == mpack_ok);
    mpack_tree_destroy(&tree);
  }
  clock_t end = clock();
  double elapsed = (double)(end - start) / CLOCKS_PER_SEC;
  double mb = (double)(size_diff * ITERS_DIFF) / (1024.0 * 1024.0);
  perf_record_speed(res, mb / elapsed);
}

// --- Encoding Global ASTs ---
mp_zone_t global_zone;
mp_object_t ast_simple;
mp_object_t ast_hard;
mp_object_t ast_diff;

char *out_buf_simple;
char *out_buf_hard;
char *out_buf_diff;

// --- Helper for CMP to encode mp_object_t ---
static void cmp_encode_mp_object(cmp_ctx_t *cmp, const mp_object_t *obj) {
  if (!obj)
    return;
  switch (obj->type) {
  case MP_TYPE_NIL:
    cmp_write_nil(cmp);
    break;
  case MP_TYPE_BOOLEAN:
    cmp_write_bool(cmp, obj->via.boolean);
    break;
  case MP_TYPE_POSITIVE_INTEGER:
    cmp_write_uinteger(cmp, obj->via.u64);
    break;
  case MP_TYPE_NEGATIVE_INTEGER:
    cmp_write_integer(cmp, obj->via.i64);
    break;
  case MP_TYPE_FLOAT32:
    cmp_write_float(cmp, obj->via.f32);
    break;
  case MP_TYPE_FLOAT64:
    cmp_write_double(cmp, obj->via.f64);
    break;
  case MP_TYPE_STR:
    cmp_write_str(cmp, obj->via.str.ptr, obj->via.str.size);
    break;
  case MP_TYPE_BIN:
    cmp_write_bin(cmp, obj->via.bin.ptr, obj->via.bin.size);
    break;
  case MP_TYPE_ARRAY:
    cmp_write_array(cmp, obj->via.array.size);
    for (uint32_t i = 0; i < obj->via.array.size; i++) {
      cmp_encode_mp_object(cmp, &obj->via.array.ptr[i]);
    }
    break;
  case MP_TYPE_MAP:
    cmp_write_map(cmp, obj->via.map.size);
    for (uint32_t i = 0; i < obj->via.map.size; i++) {
      cmp_encode_mp_object(cmp, &obj->via.map.ptr[i].key);
      cmp_encode_mp_object(cmp, &obj->via.map.ptr[i].val);
    }
    break;
  case MP_TYPE_EXT:
    cmp_write_ext(cmp, obj->via.ext.type, obj->via.ext.size, obj->via.ext.ptr);
    break;
  }
}

static void mpack_encode_mp_object(mpack_writer_t *writer,
                                   const mp_object_t *obj) {
  if (!obj)
    return;
  switch (obj->type) {
  case MP_TYPE_NIL:
    mpack_write_nil(writer);
    break;
  case MP_TYPE_BOOLEAN:
    mpack_write_bool(writer, obj->via.boolean);
    break;
  case MP_TYPE_POSITIVE_INTEGER:
    mpack_write_u64(writer, obj->via.u64);
    break;
  case MP_TYPE_NEGATIVE_INTEGER:
    mpack_write_i64(writer, obj->via.i64);
    break;
  case MP_TYPE_FLOAT32:
    mpack_write_float(writer, obj->via.f32);
    break;
  case MP_TYPE_FLOAT64:
    mpack_write_double(writer, obj->via.f64);
    break;
  case MP_TYPE_STR:
    mpack_write_str(writer, obj->via.str.ptr, obj->via.str.size);
    break;
  case MP_TYPE_BIN:
    mpack_write_bin(writer, obj->via.bin.ptr, obj->via.bin.size);
    break;
  case MP_TYPE_ARRAY:
    mpack_start_array(writer, obj->via.array.size);
    for (uint32_t i = 0; i < obj->via.array.size; i++) {
      mpack_encode_mp_object(writer, &obj->via.array.ptr[i]);
    }
    mpack_finish_array(writer);
    break;
  case MP_TYPE_MAP:
    mpack_start_map(writer, obj->via.map.size);
    for (uint32_t i = 0; i < obj->via.map.size; i++) {
      mpack_encode_mp_object(writer, &obj->via.map.ptr[i].key);
      mpack_encode_mp_object(writer, &obj->via.map.ptr[i].val);
    }
    mpack_finish_map(writer);
    break;
  case MP_TYPE_EXT:
    mpack_write_ext(writer, obj->via.ext.type, obj->via.ext.ptr,
                    obj->via.ext.size);
    break;
  }
}

// cmp writer context
static size_t cmp_mem_writer(cmp_ctx_t *ctx, const void *data, size_t count) {
  cmp_mem_t *mem = (cmp_mem_t *)ctx->buf;
  if (mem->offset + count > mem->size)
    return 0;
  memcpy(mem->buf + mem->offset, data, count);
  mem->offset += count;
  return count;
}

// --- Encoding Simple ---

void enc_simple_cmp(perf_result_t *res) {
  clock_t start = clock();
  for (int i = 0; i < ITERS_SIMPLE; i++) {
    cmp_mem_t mem = {.buf = out_buf_simple, .size = size_simple, .offset = 0};
    cmp_ctx_t cmp;
    cmp_init(&cmp, &mem, NULL, NULL, cmp_mem_writer);
    cmp_encode_mp_object(&cmp, &ast_simple);
    assert(cmp.error == 0);
    assert(mem.offset == size_simple);
  }
  clock_t end = clock();
  double elapsed = (double)(end - start) / CLOCKS_PER_SEC;
  double mb = (double)(size_simple * ITERS_SIMPLE) / (1024.0 * 1024.0);
  perf_record_speed(res, mb / elapsed);
}

void enc_simple_radikant(perf_result_t *res) {
  clock_t start = clock();
  for (int i = 0; i < ITERS_SIMPLE; i++) {
    mp_stream_buffer_t mem_buffer;
    mp_stream_t stream;
    mp_stream_init_write(&stream, &mem_buffer, false, out_buf_simple, size_simple);
    mp_error_t err = mp_encode_object(&stream, &ast_simple);
    assert(err == MP_OK);
    assert(mem_buffer.size == size_simple);
  }
  clock_t end = clock();
  double elapsed = (double)(end - start) / CLOCKS_PER_SEC;
  double mb = (double)(size_simple * ITERS_SIMPLE) / (1024.0 * 1024.0);
  perf_record_speed(res, mb / elapsed);
}

// --- Encoding Hard ---

void enc_hard_cmp(perf_result_t *res) {
  clock_t start = clock();
  for (int i = 0; i < ITERS_HARD; i++) {
    cmp_mem_t mem = {.buf = out_buf_hard, .size = size_hard, .offset = 0};
    cmp_ctx_t cmp;
    cmp_init(&cmp, &mem, NULL, NULL, cmp_mem_writer);
    cmp_encode_mp_object(&cmp, &ast_hard);
    assert(cmp.error == 0);
    assert(mem.offset == size_hard);
  }
  clock_t end = clock();
  double elapsed = (double)(end - start) / CLOCKS_PER_SEC;
  double mb = (double)(size_hard * ITERS_HARD) / (1024.0 * 1024.0);
  perf_record_speed(res, mb / elapsed);
}

void enc_hard_radikant(perf_result_t *res) {
  clock_t start = clock();
  for (int i = 0; i < ITERS_HARD; i++) {
    mp_stream_buffer_t mem_buffer;
    mp_stream_t stream;
    mp_stream_init_write(&stream, &mem_buffer, false, out_buf_hard, size_hard);
    mp_error_t err = mp_encode_object(&stream, &ast_hard);
    assert(err == MP_OK);
    assert(mem_buffer.size == size_hard);
  }
  clock_t end = clock();
  double elapsed = (double)(end - start) / CLOCKS_PER_SEC;
  double mb = (double)(size_hard * ITERS_HARD) / (1024.0 * 1024.0);
  perf_record_speed(res, mb / elapsed);
}

// --- Encoding Difficult ---

void enc_diff_cmp(perf_result_t *res) {
  clock_t start = clock();
  for (int i = 0; i < ITERS_DIFF; i++) {
    cmp_mem_t mem = {.buf = out_buf_diff, .size = size_diff, .offset = 0};
    cmp_ctx_t cmp;
    cmp_init(&cmp, &mem, NULL, NULL, cmp_mem_writer);
    cmp_encode_mp_object(&cmp, &ast_diff);
    assert(cmp.error == 0);
    assert(mem.offset == size_diff);
  }
  clock_t end = clock();
  double elapsed = (double)(end - start) / CLOCKS_PER_SEC;
  double mb = (double)(size_diff * ITERS_DIFF) / (1024.0 * 1024.0);
  perf_record_speed(res, mb / elapsed);
}

void enc_diff_radikant(perf_result_t *res) {
  clock_t start = clock();
  for (int i = 0; i < ITERS_DIFF; i++) {
    mp_stream_buffer_t mem_buffer;
    mp_stream_t stream;
    mp_stream_init_write(&stream, &mem_buffer, false, out_buf_diff, size_diff);
    mp_error_t err = mp_encode_object(&stream, &ast_diff);
    assert(err == MP_OK);
    assert(mem_buffer.size == size_diff);
  }
  clock_t end = clock();
  double elapsed = (double)(end - start) / CLOCKS_PER_SEC;
  double mb = (double)(size_diff * ITERS_DIFF) / (1024.0 * 1024.0);
  perf_record_speed(res, mb / elapsed);
}

void enc_simple_mpack(perf_result_t *res) {
  clock_t start = clock();
  for (int i = 0; i < ITERS_SIMPLE; i++) {
    mpack_writer_t writer;
    mpack_writer_init(&writer, out_buf_simple, size_simple);
    mpack_encode_mp_object(&writer, &ast_simple);
    size_t used = mpack_writer_buffer_used(&writer);
    mpack_error_t err = mpack_writer_destroy(&writer);
    assert(err == mpack_ok);
    assert(used == size_simple);
  }
  clock_t end = clock();
  double elapsed = (double)(end - start) / CLOCKS_PER_SEC;
  double mb = (double)(size_simple * ITERS_SIMPLE) / (1024.0 * 1024.0);
  perf_record_speed(res, mb / elapsed);
}

void enc_hard_mpack(perf_result_t *res) {
  clock_t start = clock();
  for (int i = 0; i < ITERS_HARD; i++) {
    mpack_writer_t writer;
    mpack_writer_init(&writer, out_buf_hard, size_hard);
    mpack_encode_mp_object(&writer, &ast_hard);
    size_t used = mpack_writer_buffer_used(&writer);
    mpack_error_t err = mpack_writer_destroy(&writer);
    assert(err == mpack_ok);
    assert(used == size_hard);
  }
  clock_t end = clock();
  double elapsed = (double)(end - start) / CLOCKS_PER_SEC;
  double mb = (double)(size_hard * ITERS_HARD) / (1024.0 * 1024.0);
  perf_record_speed(res, mb / elapsed);
}

void enc_diff_mpack(perf_result_t *res) {
  clock_t start = clock();
  for (int i = 0; i < ITERS_DIFF; i++) {
    mpack_writer_t writer;
    mpack_writer_init(&writer, out_buf_diff, size_diff);
    mpack_encode_mp_object(&writer, &ast_diff);
    size_t used = mpack_writer_buffer_used(&writer);
    mpack_error_t err = mpack_writer_destroy(&writer);
    assert(err == mpack_ok);
    assert(used == size_diff);
  }
  clock_t end = clock();
  double elapsed = (double)(end - start) / CLOCKS_PER_SEC;
  double mb = (double)(size_diff * ITERS_DIFF) / (1024.0 * 1024.0);
  perf_record_speed(res, mb / elapsed);
}

void skip_simple_radikant(perf_result_t *res) {
  clock_t start = clock();
  for (int i = 0; i < ITERS_SIMPLE; i++) {
    mp_error_t err = mp_skip_memory(buf_simple, size_simple);
    assert(err == MP_OK);
  }
  clock_t end = clock();
  double elapsed = (double)(end - start) / CLOCKS_PER_SEC;
  double mb = (double)(size_simple * ITERS_SIMPLE) / (1024.0 * 1024.0);
  perf_record_speed(res, mb / elapsed);
}

void skip_hard_radikant(perf_result_t *res) {
  clock_t start = clock();
  for (int i = 0; i < ITERS_HARD; i++) {
    mp_error_t err = mp_skip_memory(buf_hard, size_hard);
    assert(err == MP_OK);
  }
  clock_t end = clock();
  double elapsed = (double)(end - start) / CLOCKS_PER_SEC;
  double mb = (double)(size_hard * ITERS_HARD) / (1024.0 * 1024.0);
  perf_record_speed(res, mb / elapsed);
}

void skip_diff_radikant(perf_result_t *res) {
  clock_t start = clock();
  for (int i = 0; i < ITERS_DIFF; i++) {
    mp_error_t err = mp_skip_memory(buf_diff, size_diff);
    assert(err == MP_OK);
  }
  clock_t end = clock();
  double elapsed = (double)(end - start) / CLOCKS_PER_SEC;
  double mb = (double)(size_diff * ITERS_DIFF) / (1024.0 * 1024.0);
  perf_record_speed(res, mb / elapsed);
}

int main() {
  buf_simple = read_file_to_mem(
      PROJECT_ROOT "/test/vectors/test/simple/simple1.bin", &size_simple);
  buf_hard = read_file_to_mem(PROJECT_ROOT "/test/vectors/test/hard/hard1.bin",
                              &size_hard);
  buf_diff = read_file_to_mem(
      PROJECT_ROOT "/test/vectors/test/difficult/diff1.bin", &size_diff);

  if (!buf_simple || !buf_hard || !buf_diff) {
    printf("Failed to load test vectors from %s/test/vectors/...\n",
           PROJECT_ROOT);
    printf("Ensure GENERATE-VECTORS has been run first.\n");
    return 1;
  }

  // Parse files into ASTs for encoding benchmarks
  mp_zone_init(&global_zone, 1024 * 1024); // 1 MB zone for ASTs
  mp_parse_memory(&global_zone, buf_simple, size_simple, &ast_simple);
  mp_parse_memory(&global_zone, buf_hard, size_hard, &ast_hard);
  mp_parse_memory(&global_zone, buf_diff, size_diff, &ast_diff);

  // Allocate output buffers for encoding benchmarks
  out_buf_simple = (char *)malloc(size_simple);
  out_buf_hard = (char *)malloc(size_hard);
  out_buf_diff = (char *)malloc(size_diff);

  perf_suite_t enc_suite;
  init_perf_suite(&enc_suite, "MessagePack Encoding Performance");

  add_perf_test(&enc_suite, false, true, "SIMPLE VECTOR", "cmp",
                enc_simple_cmp);
  add_perf_test(&enc_suite, false, true, "SIMPLE VECTOR", "mpack",
                enc_simple_mpack);
  add_perf_test(&enc_suite, true, true, "SIMPLE VECTOR", "radikant",
                enc_simple_radikant);

  add_perf_test(&enc_suite, false, true, "HARD VECTOR", "cmp", enc_hard_cmp);
  add_perf_test(&enc_suite, false, true, "HARD VECTOR", "mpack",
                enc_hard_mpack);
  add_perf_test(&enc_suite, true, true, "HARD VECTOR", "radikant",
                enc_hard_radikant);

  add_perf_test(&enc_suite, false, true, "DIFFICULT VECTOR", "cmp",
                enc_diff_cmp);
  add_perf_test(&enc_suite, false, true, "DIFFICULT VECTOR", "mpack",
                enc_diff_mpack);
  add_perf_test(&enc_suite, true, true, "DIFFICULT VECTOR", "radikant",
                enc_diff_radikant);

  run_perf_suite(&enc_suite);
  free_perf_suite(&enc_suite);

  perf_suite_t dec_suite;
  init_perf_suite(&dec_suite, "MessagePack Decoding Performance");

  add_perf_test(&dec_suite, false, true, "SIMPLE VECTOR", "cmp",
                comp_simple_cmp);
  add_perf_test(&dec_suite, false, true, "SIMPLE VECTOR", "mpack",
                comp_simple_mpack);
  add_perf_test(&dec_suite, true, true, "SIMPLE VECTOR", "radikant",
                comp_simple_radikant);

  add_perf_test(&dec_suite, false, true, "HARD VECTOR", "cmp", comp_hard_cmp);
  add_perf_test(&dec_suite, false, true, "HARD VECTOR", "mpack",
                comp_hard_mpack);
  add_perf_test(&dec_suite, true, true, "HARD VECTOR", "radikant",
                comp_hard_radikant);

  add_perf_test(&dec_suite, false, true, "DIFFICULT VECTOR", "cmp",
                comp_diff_cmp);
  add_perf_test(&dec_suite, false, true, "DIFFICULT VECTOR", "mpack",
                comp_diff_mpack);
  add_perf_test(&dec_suite, true, true, "DIFFICULT VECTOR", "radikant",
                comp_diff_radikant);

  run_perf_suite(&dec_suite);
  free_perf_suite(&dec_suite);

  perf_suite_t skip_suite;
  init_perf_suite(&skip_suite, "MessagePack Skipping Performance");

  add_perf_test(&skip_suite, false, true, "SIMPLE VECTOR", "cmp",
                comp_simple_cmp); // CMP decodes by skipping
  add_perf_test(&skip_suite, true, true, "SIMPLE VECTOR", "radikant",
                skip_simple_radikant);

  add_perf_test(&skip_suite, false, true, "HARD VECTOR", "cmp", comp_hard_cmp);
  add_perf_test(&skip_suite, true, true, "HARD VECTOR", "radikant",
                skip_hard_radikant);

  add_perf_test(&skip_suite, false, true, "DIFFICULT VECTOR", "cmp",
                comp_diff_cmp);
  add_perf_test(&skip_suite, true, true, "DIFFICULT VECTOR", "radikant",
                skip_diff_radikant);

  run_perf_suite(&skip_suite);
  free_perf_suite(&skip_suite);

  free(buf_simple);
  free(buf_hard);
  free(buf_diff);
  free(out_buf_simple);
  free(out_buf_hard);
  free(out_buf_diff);
  mp_zone_destroy(&global_zone);
  return 0;
}
