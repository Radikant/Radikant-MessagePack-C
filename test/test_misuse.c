#include <radikant-messagepack-c.h>
#include <radikant-probe-c.h>
#include <string.h>

#include "../SRC/encoder.h"
#include "../SRC/builder.h"

// ---------------------------------------------------------
// Misuse: Encoder APIs
// ---------------------------------------------------------
bool test_misuse_encoder(test_result_t *test) {
  // Calling encoder with NULL stream
  mp_error_t err = mp_encode_int(NULL, 12345);
  if (err != MP_ERROR_BAD_ARG)
    append_error(test, "mp_encode_int(NULL) should return BAD_ARG", err);

  return test_end(test);
}

// ---------------------------------------------------------
// Misuse: Decoder APIs
// ---------------------------------------------------------
bool test_misuse_decoder(test_result_t *test) {
  mp_object_t obj;
  mp_decoder_t decoder;

  // mp_decode with NULL decoder
  mp_error_t err = mp_decode(NULL, &obj);
  if (err != MP_ERROR_BAD_ARG)
    append_error(test, "mp_decode(NULL, obj) should return BAD_ARG", err);

  // mp_decode with NULL object
  // Wait, let's initialize decoder first
  mp_stream_t stream;
  mp_stream_buffer_t buffer;
  mp_stream_init_read(&stream, &buffer, "hello", 5);
  mp_zone_t zone;
  mp_zone_init(&zone, 1024);

  mp_decoder_init(&decoder, &stream, &zone);
  err = mp_decode(&decoder, NULL);
  if (err != MP_ERROR_BAD_ARG)
    append_error(test, "mp_decode(decoder, NULL) should return BAD_ARG", err);

  mp_zone_destroy(&zone);
  return test_end(test);
}

// ---------------------------------------------------------
// Misuse: SAX API
// ---------------------------------------------------------
bool test_misuse_sax(test_result_t *test) {
  int64_t out_int;
  mp_error_t err = mp_decode_stream_int(NULL, &out_int);
  if (err != MP_ERROR_BAD_ARG)
    append_error(test, "mp_decode_stream_int(NULL) should return BAD_ARG", err);

  return test_end(test);
}

// ---------------------------------------------------------
// Misuse: Builder API
// ---------------------------------------------------------
bool test_misuse_builder(test_result_t *test) {
  mp_zone_t zone;
  mp_zone_init(&zone, 1024);
  mp_object_t obj;

  mp_error_t err = mp_build_array(NULL, &obj, 5);
  if (err != MP_ERROR_BAD_ARG)
    append_error(test, "mp_build_array(NULL zone) should return BAD_ARG", err);

  err = mp_build_array(&zone, NULL, 5);
  if (err != MP_ERROR_BAD_ARG)
    append_error(test, "mp_build_array(NULL obj) should return BAD_ARG", err);

  // Out of bounds setter misuse
  mp_build_array(&zone, &obj, 2); // Array of size 2
  mp_object_t item;
  mp_build_int(&item, 42);

  // Try setting index 5 on an array of size 2
  err = mp_array_set(&obj, 5, item);
  if (err != MP_ERROR_BAD_ARG)
    append_error(test, "mp_array_set out-of-bounds should return BAD_ARG", err);

  // Try setting an element on a non-array object
  mp_object_t not_array;
  mp_build_int(&not_array, 42);
  err = mp_array_set(&not_array, 0, item);
  if (err != MP_ERROR_BAD_ARG)
    append_error(test, "mp_array_set on INT object should return BAD_ARG", err);

  mp_zone_destroy(&zone);
  return test_end(test);
}

// ---------------------------------------------------------
// Misuse: Object API
// ---------------------------------------------------------
bool test_misuse_object(test_result_t *test) {
  mp_object_t obj;
  obj.type = MP_TYPE_POSITIVE_INTEGER;

  // Try to get array elements from an INT object
  mp_object_t *elements;
  uint32_t len;
  mp_error_t err = mp_object_as_array(&obj, &elements, &len);
  if (err != MP_ERROR_OBJECT_TYPE_MISMATCH)
    append_error(
        test, "Getting array elements from INT should fail with TYPE_MISMATCH",
        err);

  // Try passing NULL pointers
  err = mp_object_as_array(&obj, NULL, &len);
  if (err != MP_ERROR_BAD_ARG)
    append_error(test, "Passing NULL array pointer should fail", err);

  // Out of bounds getter misuse
  mp_zone_t zone;
  mp_zone_init(&zone, 1024);
  mp_object_t arr;
  mp_build_array(&zone, &arr, 2);

  // Try getting index 5 from an array of size 2
  mp_object_t* ptr = mp_array_get(&arr, 5);
  if (ptr != NULL)
    append_error(test, "mp_array_get out-of-bounds should return NULL", 0);

  // Try getting an element from a non-array
  ptr = mp_array_get(&obj, 0); // obj is MP_TYPE_POSITIVE_INTEGER
  if (ptr != NULL)
    append_error(test, "mp_array_get on INT object should return NULL", 0);

  mp_zone_destroy(&zone);
  return test_end(test);
}

// ---------------------------------------------------------
// Misuse: Zone Allocator (Will segfault on failure)
// ---------------------------------------------------------
bool test_misuse_zone(test_result_t *test) {
  // Calling mp_zone_alloc with a NULL zone.
  // If the library doesn't check for NULL, this will cause a SEGFAULT.
  void *ptr = mp_zone_alloc(NULL, 100);
  if (ptr != NULL)
    append_error(test, "Allocating from NULL zone should return NULL", 0);

  return test_end(test);
}

bool test_misuse_stream(test_result_t *test) {
  const char data[] = "hello";
  mp_stream_t stream;
  mp_stream_buffer_t buffer;

  // 1. Initialize for reading
  mp_stream_init_read(&stream, &buffer, data, 5);

  // Misuse 1: Passing a NULL buffer to read
  mp_error_t err = stream.read(&stream, NULL, 5);
  if (err != MP_ERROR_BAD_ARG)
    append_error(test, "Expected MP_ERROR_BAD_ARG on NULL buffer", err);

  // Misuse 2: Trying to write to a read-only stream
  // WARNING: stream.write is NULL for a read stream. Calling it directly would
  // segfault! Users must check if the function pointer is non-NULL or rely on
  // higher-level wrappers.
  if (stream.write != NULL)
    append_error(test, "Read-only stream should have NULL write function", 0);

  // 2. Initialize for writing (dynamic)
  mp_stream_init_write(&stream, &buffer, true, NULL, 0);

  // Misuse 3: Trying to read from a write-only stream
  if (stream.read != NULL)
    append_error(test, "Write-only stream should have NULL read function", 0);

  // Misuse 4: Write with a NULL buffer but positive count
  err = stream.write(&stream, NULL, 5);
  if (err != MP_ERROR_BAD_ARG)
    append_error(test, "Expected MP_ERROR_BAD_ARG on NULL write buffer", err);

  mp_memory_stream_destroy(&buffer);

  return test_end(test);
}

test_suite_t suite_misuse = {.name = "MessagePack Misuse Suite",
                             .standard = "MsgPack-Misuse"};

int main(void) {
  r_set_global_verbosity(R_VERBOSE);
  enable_memleak_detection(&suite_misuse);

  add_test(&suite_misuse, test_misuse_encoder, "Misuse: Encoder", "Misuse");
  add_test(&suite_misuse, test_misuse_decoder, "Misuse: Decoder", "Misuse");
  add_test(&suite_misuse, test_misuse_sax, "Misuse: SAX API", "Misuse");
  add_test(&suite_misuse, test_misuse_builder, "Misuse: Builder API", "Misuse");
  add_test(&suite_misuse, test_misuse_object, "Misuse: Object API", "Misuse");
  add_test(&suite_misuse, test_misuse_stream, "Misuse: Stream API", "Misuse");

  // Uncommenting this test will crash procces due to missing NULL checks (but
  // is perfectly acceptable) add_test(&suite_misuse, test_misuse_zone, "Misuse:
  // Zone Allocator", "Misuse");

  register_suite(&suite_misuse);
  bool success = run_all_suites();
  return success ? 0 : 1;
}
