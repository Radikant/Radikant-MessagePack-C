#include "../include/radikant-messagepack-c.h"
#include "probe.h"
#include <stdio.h>
#include <string.h>
#include <stdint.h>

bool msgpack_test_security_array_overflow(test_result_t *test);
bool msgpack_test_security_map_overflow(test_result_t *test);
bool msgpack_test_security_skip_desync(test_result_t *test);

test_suite_t suite_security = {
    .name = "MessagePack Adversarial Security Suite",
    .standard = "MsgPack-Protocol"
};

int main(void) {
    r_set_global_verbosity(R_VERBOSE);
    enable_memleak_detection(&suite_security);

    add_test(&suite_security, msgpack_test_security_array_overflow, "Array32 32-bit Integer Overflow", "MsgPack-Spec");
    add_test(&suite_security, msgpack_test_security_map_overflow, "Map32 32-bit Integer Overflow", "MsgPack-Spec");
    add_test(&suite_security, msgpack_test_security_skip_desync, "Skip Map32 Stream Desync Overflow", "MsgPack-Spec");

    register_suite(&suite_security);
    bool success = run_all_suites();

    free_suite(&suite_security);
    return success ? 0 : 1;
}

bool msgpack_test_security_array_overflow(test_result_t *test) {
    // Array32 tag (0xdd) + length 0xFFFFFFFF
    const char malicious_payload[] = {
        (char)0xdd, 
        (char)0xff, (char)0xff, (char)0xff, (char)0xff
    };

    mp_zone_t zone;
    mp_zone_init(&zone, 1024);

    mp_object_t out_obj;
    mp_error_t err = mp_parse_memory(&zone, malicious_payload, sizeof(malicious_payload), &out_obj);

    if (err != MP_ERROR_NOMEM && err != MP_ERROR_DECODE_INCOMPLETE) {
        char err_msg[128];
        snprintf(err_msg, sizeof(err_msg), "Failed to catch Array32 length overflow. Expected MP_ERROR_NOMEM or MP_ERROR_DECODE_INCOMPLETE, got: %d", err);
        append_error(test, err_msg, 0);
    }

    mp_zone_destroy(&zone);
    return test_end(test);
}

bool msgpack_test_security_map_overflow(test_result_t *test) {
    // Map32 tag (0xdf) + length 0xFFFFFFFF
    const char malicious_payload[] = {
        (char)0xdf, 
        (char)0xff, (char)0xff, (char)0xff, (char)0xff
    };

    mp_zone_t zone;
    mp_zone_init(&zone, 1024);

    mp_object_t out_obj;
    mp_error_t err = mp_parse_memory(&zone, malicious_payload, sizeof(malicious_payload), &out_obj);

    if (err != MP_ERROR_NOMEM && err != MP_ERROR_DECODE_INCOMPLETE) {
        char err_msg[128];
        snprintf(err_msg, sizeof(err_msg), "Failed to catch Map32 length overflow. Expected MP_ERROR_NOMEM or MP_ERROR_DECODE_INCOMPLETE, got: %d", err);
        append_error(test, err_msg, 0);
    }

    mp_zone_destroy(&zone);
    return test_end(test);
}

bool msgpack_test_security_skip_desync(test_result_t *test) {
    // Map32 tag (0xdf) + length 0x80000000. 
    // This previously caused (len * 2) to overflow to 0 and incorrectly return MP_OK instantly.
    const char malicious_payload[] = {
        (char)0xdf, 
        (char)0x80, (char)0x00, (char)0x00, (char)0x00
    };

    mp_error_t err = mp_skip_memory(malicious_payload, sizeof(malicious_payload));

    if (err == MP_OK) {
        append_error(test, "Skip desync caught: skip erroneously returned MP_OK for a 2-billion element map.", 0);
    } else if (err != MP_ERROR_DECODE_INCOMPLETE) {
        append_error(test, "Expected MP_ERROR_DECODE_INCOMPLETE as it should safely hit the stream EOF.", 0);
    }

    return test_end(test);
}
