#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Radikant
#include <radikant-messagepack-c.h>
#include <radikant-probe-c.h>

#ifndef PROJECT_ROOT
#define PROJECT_ROOT "."
#endif

// Global vectors
char *buf_simple = NULL;
char *buf_hard = NULL;
char *buf_diff = NULL;
size_t size_simple = 0;
size_t size_hard = 0;
size_t size_diff = 0;

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

// ---------------------------------------------------------
// Fuzzing Strategies
// ---------------------------------------------------------

static void parse_and_destroy(const char *buf, size_t size, size_t zone_size) {
  mp_zone_t zone;
  mp_zone_init(&zone, zone_size);

  mp_object_t obj;
  // We ignore the return error. We only care that it does not crash!
  mp_parse_memory(&zone, buf, size, &obj);

  mp_zone_destroy(&zone);
}

// 1. Pure Noise
static bool test_fuzz_noise(test_result_t *res) {
  time_t start = time(NULL);
  while (time(NULL) - start < 20) {
    size_t size = (rand() % 65536) + 1; // 1 to 64KB
    char *buf = (char *)malloc(size);
    for (size_t i = 0; i < size; i++) {
      buf[i] = (char)(rand() % 256);
    }
    parse_and_destroy(buf, size, 4096);
    free(buf);
  }
  return true;
}

// 2. Corrupted Valid Data
static bool test_fuzz_corrupted(test_result_t *res) {
  time_t start = time(NULL);
  while (time(NULL) - start < 20) {
    char *src_buf;
    size_t src_size;
    int pick = rand() % 3;
    if (pick == 0) {
      src_buf = buf_simple;
      src_size = size_simple;
    } else if (pick == 1) {
      src_buf = buf_hard;
      src_size = size_hard;
    } else {
      src_buf = buf_diff;
      src_size = size_diff;
    }

    size_t size = src_size;
    int action = rand() % 3;
    if (action == 0 && src_size > 0) { // Truncate
      size = rand() % src_size;
    } else if (action == 1) { // Expand
      size += rand() % 1000;
    }

    char *buf = (char *)malloc(size);
    size_t copy_size = (size < src_size) ? size : src_size;
    memcpy(buf, src_buf, copy_size);
    for (size_t i = copy_size; i < size; i++) {
      buf[i] = (char)(rand() % 256);
    }

    if (size > 0) {
      int flips = rand() % 100;
      for (int i = 0; i < flips; i++) {
        size_t idx = rand() % size;
        buf[idx] ^= (char)(1 << (rand() % 8));
      }
    }
    parse_and_destroy(buf, size, 4096);
    free(buf);
  }
  return true;
}

// 3. Structured Fuzzing
static bool test_fuzz_structured(test_result_t *res) {
  time_t start = time(NULL);
  while (time(NULL) - start < 20) {
    size_t size = 100;
    char *buf = (char *)malloc(size);
    memset(buf, 0, size);
    size_t offset = 0;

    buf[offset++] = 0xdd; // Array 32
    buf[offset++] = 0xFF;
    buf[offset++] = 0xFF;
    buf[offset++] = 0xFF;
    buf[offset++] = 0xFF;
    buf[offset++] = 0xdb; // Str 32
    buf[offset++] = 0x7F;
    buf[offset++] = 0xFF;
    buf[offset++] = 0xFF;
    buf[offset++] = 0xFF;
    buf[offset++] = 0xdf; // Map 32
    buf[offset++] = 0xFF;
    buf[offset++] = 0xFF;
    buf[offset++] = 0xFF;
    buf[offset++] = 0xFF;

    parse_and_destroy(buf, offset, 4096);
    free(buf);
  }
  return true;
}

// 4. Deep Recursion Fuzzing
static bool test_fuzz_deep_recursion(test_result_t *res) {
  time_t start = time(NULL);
  while (time(NULL) - start < 20) {
    size_t size = 5000;
    char *buf = (char *)malloc(size);
    for (size_t i = 0; i < size - 1; i++) {
      buf[i] = (rand() % 2 == 0) ? 0x91 : 0x81;
    }
    buf[size - 1] = 0x00;
    parse_and_destroy(buf, size, 4096);
    free(buf);
  }
  return true;
}

// 5. Boundary Sizes
static bool test_fuzz_boundary(test_result_t *res) {
  time_t start = time(NULL);
  while (time(NULL) - start < 20) {
    size_t size = 128;
    char *buf = (char *)malloc(size);
    memset(buf, 0, size);
    size_t offset = 0;

    buf[offset++] = 0xcb; // Float 64 NaN
    buf[offset++] = 0x7F;
    buf[offset++] = 0xF8;
    buf[offset++] = 0x00;
    buf[offset++] = 0x00;
    buf[offset++] = 0x00;
    buf[offset++] = 0x00;
    buf[offset++] = 0x00;
    buf[offset++] = 0x00;

    buf[offset++] = 0xd3; // Int64 Min
    buf[offset++] = 0x80;
    buf[offset++] = 0x00;
    buf[offset++] = 0x00;
    buf[offset++] = 0x00;
    buf[offset++] = 0x00;
    buf[offset++] = 0x00;
    buf[offset++] = 0x00;
    buf[offset++] = 0x00;

    buf[offset++] = 0xcf; // UInt64 Max
    buf[offset++] = 0xFF;
    buf[offset++] = 0xFF;
    buf[offset++] = 0xFF;
    buf[offset++] = 0xFF;
    buf[offset++] = 0xFF;
    buf[offset++] = 0xFF;
    buf[offset++] = 0xFF;
    buf[offset++] = 0xFF;

    parse_and_destroy(buf, offset, 4096);
    free(buf);
  }
  return true;
}

// 6. Artificial OOM Fuzzing
static bool test_fuzz_oom(test_result_t *res) {
  time_t start = time(NULL);
  while (time(NULL) - start < 20) {
    parse_and_destroy(buf_diff, size_diff, 32);
  }
  return true;
}

// ---------------------------------------------------------

test_suite_t suite_fuzz = {.name = "MessagePack Fuzzing Suite (2 Minutes)",
                           .standard = "Robustness"};

int main() {
  buf_simple = read_file_to_mem(
      PROJECT_ROOT "/test/vectors/test/simple/simple1.bin", &size_simple);
  buf_hard = read_file_to_mem(PROJECT_ROOT "/test/vectors/test/hard/hard1.bin",
                              &size_hard);
  buf_diff = read_file_to_mem(
      PROJECT_ROOT "/test/vectors/test/difficult/diff1.bin", &size_diff);

  if (!buf_simple || !buf_hard || !buf_diff) {
    printf("Failed to load test vectors.\n");
    return 1;
  }

  srand((unsigned int)time(NULL));

  r_set_global_verbosity(R_VERBOSE);
  enable_memleak_detection(&suite_fuzz);

  add_test(&suite_fuzz, test_fuzz_noise, "Pure Noise (20s)", "Fuzz-Noise");
  add_test(&suite_fuzz, test_fuzz_corrupted, "Corrupted Data (20s)",
           "Fuzz-Corrupt");
  add_test(&suite_fuzz, test_fuzz_structured, "Structured Limits (20s)",
           "Fuzz-Structure");
  add_test(&suite_fuzz, test_fuzz_deep_recursion, "Deep Recursion (20s)",
           "Fuzz-Recursion");
  add_test(&suite_fuzz, test_fuzz_boundary, "Boundary & Edge Cases (20s)",
           "Fuzz-Boundary");
  add_test(&suite_fuzz, test_fuzz_oom, "Artificial OOM Constraints (20s)",
           "Fuzz-OOM");

  register_suite(&suite_fuzz);
  bool success = run_all_suites();

  free_suite(&suite_fuzz);
  free(buf_simple);
  free(buf_hard);
  free(buf_diff);
  return success ? 0 : 1;
}
