#include <stdio.h>
#include <string.h>

// cmp
#include <cmp.h>
// mpack
#include <mpack.h>
// Radikant
#include <radikant-messagepack-c.h>
#include <radikant-probe-c.h>

static size_t local_cmp_writer(cmp_ctx_t *ctx, const void *data, size_t count) {
    char **buf = (char**)ctx->buf;
    memcpy(*buf, data, count);
    *buf += count;
    return count;
}

bool test_library_equality(test_result_t *test) {
    char buf_radikant[1024];
    char buf_cmp[1024];
    char buf_mpack[1024];

    // --- Encode with Radikant ---
    mp_stream_t stream_rad;
    mp_stream_buffer_t mem_rad;
    mp_stream_init_write(&stream_rad, &mem_rad, false, buf_radikant, sizeof(buf_radikant));

    // Array of 4 elements: [true, -42, 3.14, "hello"]
    mp_encode_array_len(&stream_rad, 4);
    mp_encode_bool(&stream_rad, true);
    mp_encode_int(&stream_rad, -42);
    mp_encode_double(&stream_rad, 3.14);
    mp_encode_str(&stream_rad, "hello", 5);

    // --- Encode with CMP ---
    char *cmp_ptr = buf_cmp;
    cmp_ctx_t cmp;
    cmp_init(&cmp, &cmp_ptr, NULL, NULL, local_cmp_writer);
    cmp_write_array(&cmp, 4);
    cmp_write_bool(&cmp, true);
    cmp_write_sint(&cmp, -42);
    cmp_write_decimal(&cmp, 3.14);
    cmp_write_str(&cmp, "hello", 5);

    // --- Encode with MPack ---
    mpack_writer_t mpack;
    mpack_writer_init(&mpack, buf_mpack, sizeof(buf_mpack));
    mpack_start_array(&mpack, 4);
    mpack_write_bool(&mpack, true);
    mpack_write_i32(&mpack, -42);
    mpack_write_double(&mpack, 3.14);
    mpack_write_str(&mpack, "hello", 5);
    mpack_finish_array(&mpack);
    size_t mpack_size = mpack_writer_buffer_used(&mpack);
    mpack_writer_destroy(&mpack);

    // --- Compare ---
    size_t rad_size = mem_rad.size;
    size_t cmp_size = cmp_ptr - buf_cmp;

    if (rad_size != cmp_size) {
        append_error(test, "Radikant vs CMP size mismatch", 0);
    } else if (memcmp(buf_radikant, buf_cmp, rad_size) != 0) {
        append_error(test, "Radikant vs CMP payload mismatch", 0);
    }

    if (rad_size != mpack_size) {
        append_error(test, "Radikant vs MPack size mismatch", 0);
    } else if (memcmp(buf_radikant, buf_mpack, rad_size) != 0) {
        append_error(test, "Radikant vs MPack payload mismatch", 0);
    }

    return test_end(test);
}

test_suite_t suite_equal = { .name = "Cross-Library Equality Suite", .standard = "MsgPack-Spec" };

int main(void) {
    r_set_global_verbosity(R_VERBOSE);
    enable_memleak_detection(&suite_equal);

    add_test(&suite_equal, test_library_equality, "Library Encode Equality", "MsgPack-Spec");

    bool success = run_single_suite(&suite_equal);
    free_suite(&suite_equal);
    return success ? 0 : 1;
}
