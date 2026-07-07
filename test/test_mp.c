#include "../SRC/messagepack.h"
#include "probe.h"
#include <stdio.h>
#include <string.h>

bool msgpack_test_encode_decode(test_result_t *test);

test_suite_t suite_msgpack = {
    .name = "MessagePack Stream/API Suite",
    .standard = "MsgPack-Protocol"
};

int main(void) {
    r_set_global_verbosity(R_VERBOSE);
    enable_memleak_detection(&suite_msgpack);

    add_test(&suite_msgpack, msgpack_test_encode_decode, "Encode/Decode via Streams", "MsgPack-Spec");

    register_suite(&suite_msgpack);
    bool success = run_all_suites();

    free_suite(&suite_msgpack);
    return success ? 0 : 1;
}

bool msgpack_test_encode_decode(test_result_t *test) {
    mp_stream_t write_stream;
    mp_memory_stream_ctx_t write_ctx;
    mp_memory_stream_init_write_dynamic(&write_stream, &write_ctx);

    // Encode
    mp_encode_array_len(&write_stream, 5);
    mp_encode_int(&write_stream, 1);
    mp_encode_str(&write_stream, "hello", 5);
    mp_encode_bool(&write_stream, true);
    mp_encode_int(&write_stream, -42);
    mp_encode_float(&write_stream, 3.14f);

    // Decode Top-Level API
    mp_zone_t zone;
    mp_zone_init(&zone, 4096);
    
    mp_object_t obj;
    mp_error_t err = mp_parse_memory(&zone, write_ctx.data, write_ctx.size, &obj);
    
    if (err != MP_OK) {
        append_error(test, "Decode failed with error code", err);
    } else {
        mp_object_t* elements;
        uint32_t len;
        
        if (mp_object_as_array(&obj, &elements, &len) != MP_OK) {
            append_error(test, "Expected ARRAY type", obj.type);
        } else if (len != 5) {
            append_error(test, "Expected array size 5", len);
        } else {
            int64_t i_val;
            if (mp_object_as_int(&elements[0], &i_val) != MP_OK || i_val != 1) {
                append_error(test, "Index 0: Expected integer 1", 0);
            }

            const char* s_val;
            uint32_t s_len;
            if (mp_object_as_str(&elements[1], &s_val, &s_len) != MP_OK || s_len != 5 || strncmp(s_val, "hello", 5) != 0) {
                append_error(test, "Index 1: Expected string 'hello'", 0);
            }

            bool b_val;
            if (mp_object_as_bool(&elements[2], &b_val) != MP_OK || b_val != true) {
                append_error(test, "Index 2: Expected bool true", 0);
            }

            if (mp_object_as_int(&elements[3], &i_val) != MP_OK || i_val != -42) {
                append_error(test, "Index 3: Expected integer -42", 0);
            }

            float f_val;
            if (mp_object_as_float(&elements[4], &f_val) != MP_OK) {
                append_error(test, "Index 4: Expected FLOAT32", 0);
            }
        }
    }

    mp_zone_destroy(&zone);
    mp_memory_stream_destroy(&write_ctx);

    return test_end(test);
}