#include <radikant-messagepack-c.h>
#include <radikant-probe-c.h>
#include <string.h>

bool test_stream_memory_read(test_result_t *test) {
    const char data[] = "hello world";
    mp_stream_t stream;
    mp_memory_stream_ctx_t ctx;
    mp_stream_init_read(&stream, &ctx, data, sizeof(data) - 1); // exclude null terminator
    
    char buf[5];
    mp_error_t err = stream.read(&stream, buf, 5);
    if (err != MP_OK) append_error(test, "Failed to read 5 bytes", err);
    if (memcmp(buf, "hello", 5) != 0) append_error(test, "Read data mismatch", 0);
    
    return test_end(test);
}

bool test_stream_memory_read_eof(test_result_t *test) {
    const char data[] = "hi";
    mp_stream_t stream;
    mp_memory_stream_ctx_t ctx;
    mp_stream_init_read(&stream, &ctx, data, 2);
    
    char buf[5];
    mp_error_t err = stream.read(&stream, buf, 5);
    if (err != MP_ERROR_DECODE_INCOMPLETE) append_error(test, "Expected incomplete error", err);
    
    return test_end(test);
}

bool test_stream_memory_skip(test_result_t *test) {
    const char data[] = "0123456789";
    mp_stream_t stream;
    mp_memory_stream_ctx_t ctx;
    mp_stream_init_read(&stream, &ctx, data, 10);
    
    mp_error_t err = stream.skip(&stream, 5);
    if (err != MP_OK) append_error(test, "Failed to skip 5 bytes", err);
    
    char buf[2];
    stream.read(&stream, buf, 2);
    if (memcmp(buf, "56", 2) != 0) append_error(test, "Skip offset mismatch", 0);
    
    return test_end(test);
}

bool test_stream_memory_write(test_result_t *test) {
    mp_stream_t stream;
    mp_memory_stream_ctx_t ctx;
    mp_stream_init_write(&stream, &ctx, true, NULL, 0); // Dynamic alloc
    
    mp_error_t err = stream.write(&stream, "hello", 5);
    if (err != MP_OK) append_error(test, "Failed to write", err);
    
    err = stream.write(&stream, " world", 6);
    if (err != MP_OK) append_error(test, "Failed to write", err);
    
    if (ctx.size != 11) append_error(test, "Written size mismatch", ctx.size);
    if (memcmp(ctx.data, "hello world", 11) != 0) append_error(test, "Written data mismatch", 0);
    
    mp_memory_stream_destroy(&ctx);
    return test_end(test);
}

bool test_stream_api_misuse(test_result_t *test) {
    const char data[] = "hello";
    mp_stream_t stream;
    mp_memory_stream_ctx_t ctx;
    
    // 1. Initialize for reading
    mp_stream_init_read(&stream, &ctx, data, 5);
    
    // Misuse 1: Passing a NULL buffer to read
    mp_error_t err = stream.read(&stream, NULL, 5);
    if (err != MP_ERROR_BAD_ARG) append_error(test, "Expected MP_ERROR_BAD_ARG on NULL buffer", err);
    
    // Misuse 2: Trying to write to a read-only stream
    // WARNING: stream.write is NULL for a read stream. Calling it directly would segfault!
    // Users must check if the function pointer is non-NULL or rely on higher-level wrappers.
    if (stream.write != NULL) append_error(test, "Read-only stream should have NULL write function", 0);
    
    // 2. Initialize for writing (dynamic)
    mp_stream_init_write(&stream, &ctx, true, NULL, 0);
    
    // Misuse 3: Trying to read from a write-only stream
    if (stream.read != NULL) append_error(test, "Write-only stream should have NULL read function", 0);
    
    // Misuse 4: Write with a NULL buffer but positive count
    err = stream.write(&stream, NULL, 5);
    if (err != MP_ERROR_BAD_ARG) append_error(test, "Expected MP_ERROR_BAD_ARG on NULL write buffer", err);
    
    mp_memory_stream_destroy(&ctx);
    
    return test_end(test);
}

test_suite_t suite_stream = {.name = "MessagePack Stream Suite", .standard = "MsgPack-Stream"};

int main(void) {
    r_set_global_verbosity(R_VERBOSE);
    enable_memleak_detection(&suite_stream);
    
    add_test(&suite_stream, test_stream_memory_read, "Stream: Memory Read", "Stream-Spec");
    add_test(&suite_stream, test_stream_memory_read_eof, "Stream: Memory Read EOF", "Stream-Spec");
    add_test(&suite_stream, test_stream_memory_skip, "Stream: Memory Skip", "Stream-Spec");
    add_test(&suite_stream, test_stream_memory_write, "Stream: Memory Write", "Stream-Spec");
    add_test(&suite_stream, test_stream_api_misuse, "Stream: API Misuse", "Stream-Spec");
    
    register_suite(&suite_stream);
    bool success = run_all_suites();
    return success ? 0 : 1;
}
