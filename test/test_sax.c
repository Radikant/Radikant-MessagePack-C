#include "../include/radikant-messagepack-c.h"
#include "probe.h"
#include <stdio.h>
#include <string.h>

bool msgpack_test_sax_decode(test_result_t *test) {
    mp_stream_t write_stream;
    mp_memory_stream_ctx_t write_ctx;
    mp_stream_init_write(&write_stream, &write_ctx, true, NULL, 0);

    // Encode a sequence: INT, FLOAT, STR
    mp_encode_int(&write_stream, 42);
    mp_encode_float(&write_stream, 3.14f);
    mp_encode_str(&write_stream, "sax", 3);

    // Prepare read stream
    mp_stream_t read_stream;
    mp_memory_stream_ctx_t read_ctx;
    mp_stream_init_read(&read_stream, &read_ctx, write_ctx.data, write_ctx.size);

    int64_t i_val;
    if (mp_decode_stream_int(&read_stream, &i_val) != MP_OK || i_val != 42) {
        append_error(test, "SAX read INT failed", 0);
    }

    float f_val;
    if (mp_decode_stream_float(&read_stream, &f_val) != MP_OK) {
        append_error(test, "SAX read FLOAT failed", 0);
    }

    uint32_t s_len;
    if (mp_decode_stream_str_len(&read_stream, &s_len) != MP_OK || s_len != 3) {
        append_error(test, "SAX read STR length failed", 0);
    }

    char s_buf[4] = {0};
    if (read_stream.read(&read_stream, s_buf, s_len) != MP_OK || strncmp(s_buf, "sax", 3) != 0) {
        append_error(test, "SAX read STR body failed", 0);
    }

    mp_memory_stream_destroy(&write_ctx);
    return test_end(test);
}

test_suite_t suite_sax = {
    .name = "MessagePack SAX Decoding Suite",
    .standard = "MsgPack-Protocol"
};

int main(void) {
    r_set_global_verbosity(R_VERBOSE);
    enable_memleak_detection(&suite_sax);

    add_test(&suite_sax, msgpack_test_sax_decode, "Incremental SAX Decoding API", "MsgPack-Spec");

    register_suite(&suite_sax);
    bool success = run_all_suites();
    return success ? 0 : 1;
}
