#include <string.h>

// Radikant
#include <radikant-messagepack-c.h>
#include <radikant-probe-c.h>

// -----------------------------------------------------------------------------
// Stream Rewinding Tests
// -----------------------------------------------------------------------------

bool test_stream_rewind(test_result_t *test) {
    mp_zone_t zone;
    mp_zone_init(&zone, 4096);

    // Build an array of integers
    mp_object_t arr;
    mp_build_array(&zone, &arr, 3);
    mp_array_set_int(&arr, 0, 10);
    mp_array_set_int(&arr, 1, 20);
    mp_array_set_int(&arr, 2, 30);

    mp_stream_buffer_t buffer;
    mp_stream_t stream;
    mp_stream_init_write(&stream, &buffer, true, NULL, 0);
    mp_encode_object(&stream, &arr);

    // Read back
    mp_stream_t read_stream;
    mp_stream_init_read(&read_stream, &buffer, buffer.data, buffer.size);

    uint32_t len;
    if (mp_decode_stream_array_len(&read_stream, &len) != MP_OK)
        append_error(test, "Read Array Len failed", 0);
    if (len != 3)
        append_error(test, "Len is not 3", 0);

    // Mark before reading the first element
    if (mp_stream_mark(&read_stream) != MP_OK)
        append_error(test, "Mark Stream failed", 0);

    // Read first element
    int64_t val1;
    if (mp_decode_stream_int(&read_stream, &val1) != MP_OK)
        append_error(test, "Read first int failed", 0);
    if (val1 != 10)
        append_error(test, "First is not 10", 0);

    // Reset stream
    if (mp_stream_reset(&read_stream) != MP_OK)
        append_error(test, "Reset Stream failed", 0);

    // Read first element AGAIN!
    int64_t val1_again;
    if (mp_decode_stream_int(&read_stream, &val1_again) != MP_OK)
        append_error(test, "Read first int again failed", 0);
    if (val1_again != 10)
        append_error(test, "First is not 10 again", 0);

    mp_zone_destroy(&zone);
    mp_memory_stream_destroy(&buffer);
    return test_end(test);
}

bool test_stream_memory_read(test_result_t *test) {
  const char data[] = "hello world";
  mp_stream_t stream;
  mp_stream_buffer_t buffer;
  mp_stream_init_read(&stream, &buffer, data,
                      sizeof(data) - 1); // exclude null terminator

  char buf[5];
  mp_error_t err = stream.read(&stream, buf, 5);
  if (err != MP_OK)
    append_error(test, "Failed to read 5 bytes", err);
  if (memcmp(buf, "hello", 5) != 0)
    append_error(test, "Read data mismatch", 0);

  return test_end(test);
}

bool test_stream_memory_read_eof(test_result_t *test) {
  const char data[] = "hi";
  mp_stream_t stream;
  mp_stream_buffer_t buffer;
  mp_stream_init_read(&stream, &buffer, data, 2);

  char buf[5];
  mp_error_t err = stream.read(&stream, buf, 5);
  if (err != MP_ERROR_DECODE_INCOMPLETE)
    append_error(test, "Expected incomplete error", err);

  return test_end(test);
}

bool test_stream_memory_skip(test_result_t *test) {
  const char data[] = "0123456789";
  mp_stream_t stream;
  mp_stream_buffer_t buffer;
  mp_stream_init_read(&stream, &buffer, data, 10);

  mp_error_t err = stream.skip(&stream, 5);
  if (err != MP_OK)
    append_error(test, "Failed to skip 5 bytes", err);

  char buf[2];
  stream.read(&stream, buf, 2);
  if (memcmp(buf, "56", 2) != 0)
    append_error(test, "Skip offset mismatch", 0);

  return test_end(test);
}

bool test_stream_memory_write(test_result_t *test) {
  mp_stream_t stream;
  mp_stream_buffer_t buffer;
  mp_stream_init_write(&stream, &buffer, true, NULL, 0); // Dynamic alloc

  mp_error_t err = stream.write(&stream, "hello", 5);
  if (err != MP_OK)
    append_error(test, "Failed to write", err);

  err = stream.write(&stream, " world", 6);
  if (err != MP_OK)
    append_error(test, "Failed to write", err);

  if (buffer.size != 11)
    append_error(test, "Written size mismatch", buffer.size);
  if (memcmp(buffer.data, "hello world", 11) != 0)
    append_error(test, "Written data mismatch", 0);

  mp_memory_stream_destroy(&buffer);
  return test_end(test);
}

bool test_stream_api_misuse(test_result_t *test) {
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

bool test_stream_memory_skip_eof(test_result_t *test) {
  const char data[] = "hi";
  mp_stream_t stream;
  mp_stream_buffer_t buffer;
  mp_stream_init_read(&stream, &buffer, data, 2);

  mp_error_t err = stream.skip(&stream, 5);
  if (err != MP_ERROR_DECODE_INCOMPLETE)
    append_error(test, "Expected incomplete error when skipping past EOF", err);

  return test_end(test);
}

bool test_stream_static_buffer_overflow(test_result_t *test) {
  char static_buf[5];
  mp_stream_t stream;
  mp_stream_buffer_t buffer;
  // Initialize a static non-dynamic buffer of size 5
  mp_stream_init_write(&stream, &buffer, false, static_buf, 5);

  mp_error_t err = stream.write(&stream, "hello world", 11);
  if (err != MP_ERROR_ENCODE_BUFFER_OVERFLOW)
    append_error(test, "Expected MP_ERROR_ENCODE_BUFFER_OVERFLOW", err);

  return test_end(test);
}

bool test_stream_integer_overflow(test_result_t *test) {
  const char data[] = "hello";
  mp_stream_t stream;
  mp_stream_buffer_t buffer;
  mp_stream_init_read(&stream, &buffer, data, 5);

  // Ask for a massive read that would overflow if naive addition was used
  char buf[10];
  mp_error_t err = stream.read(&stream, buf, SIZE_MAX);
  if (err != MP_ERROR_DECODE_INCOMPLETE)
    append_error(test, "Expected incomplete error against integer overflow read", err);

  err = stream.skip(&stream, SIZE_MAX);
  if (err != MP_ERROR_DECODE_INCOMPLETE)
    append_error(test, "Expected incomplete error against integer overflow skip", err);

  return test_end(test);
}

bool test_stream_blind_reset(test_result_t *test) {
  const char data[] = "abc";
  mp_stream_t stream;
  mp_stream_buffer_t buffer;
  mp_stream_init_read(&stream, &buffer, data, 3);

  // Read 1 byte
  char buf[1];
  stream.read(&stream, buf, 1);
  if (buf[0] != 'a') append_error(test, "Expected 'a'", 0);

  // Reset without ever calling mark
  if (mp_stream_reset(&stream) != MP_OK)
    append_error(test, "Reset Stream failed", 0);

  // Should have safely rewound to 0
  stream.read(&stream, buf, 1);
  if (buf[0] != 'a') append_error(test, "Expected 'a' after blind reset", 0);

  return test_end(test);
}

test_suite_t suite_stream = {.name = "MessagePack Stream Suite",
                             .standard = "MsgPack-Stream"};

int main(void) {
  r_set_global_verbosity(R_VERBOSE);
  enable_memleak_detection(&suite_stream);

  add_test(&suite_stream, test_stream_memory_read, "Stream: Memory Read",
           "Stream-Spec");
  add_test(&suite_stream, test_stream_memory_read_eof,
           "Stream: Memory Read EOF", "Stream-Spec");
  add_test(&suite_stream, test_stream_memory_skip, "Stream: Memory Skip",
           "Stream-Spec");
  add_test(&suite_stream, test_stream_memory_write, "Stream: Memory Write",
           "Stream-Spec");
  add_test(&suite_stream, test_stream_rewind, "Stream: Rewind",
           "Stream-Spec");
  add_test(&suite_stream, test_stream_api_misuse, "Stream: API Misuse",
           "Stream-Spec");
  add_test(&suite_stream, test_stream_memory_skip_eof, "Stream: Memory Skip EOF",
           "Stream-Spec");
  add_test(&suite_stream, test_stream_static_buffer_overflow, "Stream: Static Buffer Overflow",
           "Stream-Spec");
  add_test(&suite_stream, test_stream_integer_overflow, "Stream: Integer Overflow",
           "Stream-Spec");
  add_test(&suite_stream, test_stream_blind_reset, "Stream: Blind Reset",
           "Stream-Spec");

  register_suite(&suite_stream);
  bool success = run_all_suites();
  return success ? 0 : 1;
}
