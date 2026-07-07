#include "../SRC/messagepack.h"
#include "../reference/cmp.h"
#include "probe.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// cmp memory reader/writer context
typedef struct {
    char *buf;
    size_t size;
    size_t offset;
} cmp_mem_t;

static bool cmp_mem_reader(cmp_ctx_t *ctx, void *data, size_t limit) {
    cmp_mem_t *mem = (cmp_mem_t*)ctx->buf;
    if (mem->offset + limit > mem->size) return false;
    memcpy(data, mem->buf + mem->offset, limit);
    mem->offset += limit;
    return true;
}

static size_t cmp_mem_writer(cmp_ctx_t *ctx, const void *data, size_t count) {
    cmp_mem_t *mem = (cmp_mem_t*)ctx->buf;
    if (mem->offset + count > mem->size) return 0;
    memcpy(mem->buf + mem->offset, data, count);
    mem->offset += count;
    return count;
}

// ---------------------------------------------------------
// Cross-Validation Tests
// ---------------------------------------------------------

bool test_cross_encode_radikant_decode_cmp(test_result_t *test);
bool test_cross_encode_cmp_decode_radikant(test_result_t *test);

// ---------------------------------------------------------
// Generated Vector Tests (Radikant)
// ---------------------------------------------------------

#ifndef PROJECT_ROOT
#define PROJECT_ROOT "."
#endif

bool test_vector_file_radikant(test_result_t *test, const char* path, mp_error_t expected_error);

bool vector_test_simple(test_result_t *test) { return test_vector_file_radikant(test, PROJECT_ROOT "/test/vectors/test/simple/simple1.bin", MP_OK); }
bool vector_test_hard(test_result_t *test) { return test_vector_file_radikant(test, PROJECT_ROOT "/test/vectors/test/hard/hard1.bin", MP_OK); }
bool vector_test_difficult(test_result_t *test) { return test_vector_file_radikant(test, PROJECT_ROOT "/test/vectors/test/difficult/diff1.bin", MP_OK); }
bool vector_test_depth_bomb(test_result_t *test) { return test_vector_file_radikant(test, PROJECT_ROOT "/test/vectors/attack/mallicious/depth_bomb.bin", MP_ERROR_DECODE_DEPTH_EXCEEDED); }
bool vector_test_truncated(test_result_t *test) { return test_vector_file_radikant(test, PROJECT_ROOT "/test/vectors/attack/mallformed/truncated.bin", MP_ERROR_DECODE_INCOMPLETE); }
bool vector_test_invalid_tag(test_result_t *test) { return test_vector_file_radikant(test, PROJECT_ROOT "/test/vectors/attack/mallformed/invalid_tag.bin", MP_ERROR_DECODE_INVALID_FORMAT); }

// ---------------------------------------------------------
// Generated Vector Tests (CMP)
// ---------------------------------------------------------

bool test_vector_file_cmp(test_result_t *test, const char* path, bool expected_success);

bool cmp_vector_test_simple(test_result_t *test) { return test_vector_file_cmp(test, PROJECT_ROOT "/test/vectors/test/simple/simple1.bin", true); }
bool cmp_vector_test_hard(test_result_t *test) { return test_vector_file_cmp(test, PROJECT_ROOT "/test/vectors/test/hard/hard1.bin", true); }
bool cmp_vector_test_difficult(test_result_t *test) { return test_vector_file_cmp(test, PROJECT_ROOT "/test/vectors/test/difficult/diff1.bin", true); }
bool cmp_vector_test_depth_bomb(test_result_t *test) { return test_vector_file_cmp(test, PROJECT_ROOT "/test/vectors/attack/mallicious/depth_bomb.bin", false); }
bool cmp_vector_test_truncated(test_result_t *test) { return test_vector_file_cmp(test, PROJECT_ROOT "/test/vectors/attack/mallformed/truncated.bin", false); }
bool cmp_vector_test_invalid_tag(test_result_t *test) { return test_vector_file_cmp(test, PROJECT_ROOT "/test/vectors/attack/mallformed/invalid_tag.bin", false); }

// ---------------------------------------------------------

test_suite_t suite_vector = {
    .name = "MessagePack Vector Suite",
    .standard = "MsgPack-Spec"
};

int main(void) {
    r_set_global_verbosity(R_VERBOSE);
    enable_memleak_detection(&suite_vector);

    add_test(&suite_vector, test_cross_encode_radikant_decode_cmp, "Cross-Validation: Encode Radikant -> Decode CMP", "MsgPack-Spec");
    add_test(&suite_vector, test_cross_encode_cmp_decode_radikant, "Cross-Validation: Encode CMP -> Decode Radikant", "MsgPack-Spec");
    
    add_test(&suite_vector, vector_test_simple, "Vector (Radikant): Simple", "MsgPack-Spec");
    add_test(&suite_vector, vector_test_hard, "Vector (Radikant): Hard", "MsgPack-Spec");
    add_test(&suite_vector, vector_test_difficult, "Vector (Radikant): Difficult", "MsgPack-Spec");
    add_test(&suite_vector, vector_test_depth_bomb, "Vector Attack (Radikant): Depth Bomb", "MsgPack-Spec");
    add_test(&suite_vector, vector_test_truncated, "Vector Attack (Radikant): Truncated", "MsgPack-Spec");
    add_test(&suite_vector, vector_test_invalid_tag, "Vector Attack (Radikant): Invalid Tag", "MsgPack-Spec");

    add_test(&suite_vector, cmp_vector_test_simple, "Vector (CMP): Simple", "MsgPack-Spec");
    add_test(&suite_vector, cmp_vector_test_hard, "Vector (CMP): Hard", "MsgPack-Spec");
    add_test(&suite_vector, cmp_vector_test_difficult, "Vector (CMP): Difficult", "MsgPack-Spec");
    add_test(&suite_vector, cmp_vector_test_depth_bomb, "Vector Attack (CMP): Depth Bomb", "MsgPack-Spec");
    add_test(&suite_vector, cmp_vector_test_truncated, "Vector Attack (CMP): Truncated", "MsgPack-Spec");
    add_test(&suite_vector, cmp_vector_test_invalid_tag, "Vector Attack (CMP): Invalid Tag", "MsgPack-Spec");

    register_suite(&suite_vector);
    bool success = run_all_suites();

    free_suite(&suite_vector);
    return success ? 0 : 1;
}

bool test_vector_file_radikant(test_result_t *test, const char* path, mp_error_t expected_error) {
    FILE* f = fopen(path, "rb");
    if (!f) {
        char err_msg[256];
        snprintf(err_msg, sizeof(err_msg), "Could not open file: %s", path);
        append_error(test, err_msg, 0);
        printf("FAILED %s: %s\n", path, err_msg);
        return test_end(test);
    }

    mp_stream_t stream;
    mp_file_stream_init(&stream, f);

    mp_zone_t zone;
    mp_zone_init(&zone, 8192);

    mp_decoder_t decoder;
    mp_decoder_init(&decoder, &stream, &zone);

    mp_object_t obj;
    mp_error_t err = mp_decode(&decoder, &obj);
    if (err != expected_error) {
        char err_msg[256];
        snprintf(err_msg, sizeof(err_msg), "Expected error %d, got %d", expected_error, err);
        append_error(test, err_msg, 0);
        printf("FAILED %s: %s\n", path, err_msg);
    }

    mp_zone_destroy(&zone);
    fclose(f);
    return test_end(test);
}

bool test_vector_file_cmp(test_result_t *test, const char* path, bool expected_success) {
    FILE* f = fopen(path, "rb");
    if (!f) {
        char err_msg[256];
        snprintf(err_msg, sizeof(err_msg), "Could not open file: %s", path);
        append_error(test, err_msg, 0);
        printf("FAILED (CMP) %s: %s\n", path, err_msg);
        return test_end(test);
    }

    // Read file into memory for CMP (CMP file operations are basic, easiest to use cmp_mem_reader)
    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    fseek(f, 0, SEEK_SET);

    char *buf = (char*)malloc(fsize);
    if(fsize > 0 && buf) {
        fread(buf, 1, fsize, f);
    }

    cmp_mem_t mem = { .buf = buf, .size = fsize, .offset = 0 };
    cmp_ctx_t cmp;
    cmp_init(&cmp, &mem, cmp_mem_reader, NULL, NULL);

    // Skip recursively to validate full structure
    bool success = cmp_skip_object_no_limit(&cmp);

    if (success != expected_success) {
        char err_msg[256];
        snprintf(err_msg, sizeof(err_msg), "Expected success %d, got %d", expected_success, success);
        append_error(test, err_msg, 0);
        printf("FAILED (CMP) %s: %s\n", path, err_msg);
    }

    free(buf);
    fclose(f);
    return test_end(test);
}

// ----- (Cross Validation Code is exactly the same below) -----

bool test_cross_encode_radikant_decode_cmp(test_result_t *test) {
    mp_stream_t stream;
    mp_memory_stream_ctx_t ctx;
    mp_memory_stream_init_write_dynamic(&stream, &ctx);

    // Encode a complex payload with Radikant
    mp_encode_array_len(&stream, 4);
    mp_encode_str(&stream, "cross-validation", 16);
    mp_encode_int(&stream, -123456);
    mp_encode_float(&stream, 3.14159f);
    mp_encode_bool(&stream, true);

    // Decode with CMP
    cmp_mem_t mem = { .buf = ctx.data, .size = ctx.size, .offset = 0 };
    cmp_ctx_t cmp;
    cmp_init(&cmp, &mem, cmp_mem_reader, NULL, NULL);

    uint32_t array_size;
    if (!cmp_read_array(&cmp, &array_size) || array_size != 4) {
        append_error(test, "CMP array size mismatch", array_size);
    }

    char str_buf[64];
    uint32_t str_size = sizeof(str_buf);
    if (!cmp_read_str(&cmp, str_buf, &str_size) || strncmp(str_buf, "cross-validation", 16) != 0) {
        append_error(test, "CMP string mismatch", 0);
    }

    int32_t i_val;
    if (!cmp_read_int(&cmp, &i_val) || i_val != -123456) {
        append_error(test, "CMP int mismatch", i_val);
    }

    float f_val;
    if (!cmp_read_float(&cmp, &f_val) || f_val != 3.14159f) { 
        append_error(test, "CMP float mismatch", 0);
    }

    bool b_val;
    if (!cmp_read_bool(&cmp, &b_val) || b_val != true) {
        append_error(test, "CMP bool mismatch", 0);
    }

    mp_memory_stream_destroy(&ctx);
    return test_end(test);
}

bool test_cross_encode_cmp_decode_radikant(test_result_t *test) {
    char raw_buf[1024];
    cmp_mem_t mem = { .buf = raw_buf, .size = sizeof(raw_buf), .offset = 0 };
    
    // Encode with CMP
    cmp_ctx_t cmp;
    cmp_init(&cmp, &mem, NULL, NULL, cmp_mem_writer);
    
    cmp_write_map(&cmp, 2);
    cmp_write_str(&cmp, "key1", 4);
    cmp_write_integer(&cmp, 987654321);
    cmp_write_str(&cmp, "key2", 4);
    cmp_write_nil(&cmp);

    // Decode with Radikant
    mp_zone_t zone;
    mp_zone_init(&zone, 4096);
    
    mp_object_t obj;
    mp_error_t err = mp_parse_memory(&zone, raw_buf, mem.offset, &obj);
    if (err != MP_OK) {
        append_error(test, "Radikant parse failed", err);
    } else {
        mp_object_kv_t* map_elements;
        uint32_t map_size;
        
        if (mp_object_as_map(&obj, &map_elements, &map_size) != MP_OK || map_size != 2) {
            append_error(test, "Radikant map size mismatch", map_size);
        } else {
            const char* k1; uint32_t l1;
            int64_t v1;
            mp_object_as_str(map_elements[0].key, &k1, &l1);
            mp_object_as_int(map_elements[0].val, &v1);
            if (strncmp(k1, "key1", 4) != 0 || v1 != 987654321) {
                append_error(test, "Radikant map pair 1 mismatch", 0);
            }

            const char* k2; uint32_t l2;
            mp_object_as_str(map_elements[1].key, &k2, &l2);
            if (strncmp(k2, "key2", 4) != 0 || mp_object_as_nil(map_elements[1].val) != MP_OK) {
                append_error(test, "Radikant map pair 2 mismatch", 0);
            }
        }
    }

    mp_zone_destroy(&zone);
    return test_end(test);
}