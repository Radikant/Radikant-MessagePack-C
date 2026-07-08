#include <radikant-messagepack-c.h>
#include <radikant-probe-c.h>

bool test_object_as_int(test_result_t *test) {
    mp_object_t obj;
    obj.type = MP_TYPE_NEGATIVE_INTEGER;
    obj.via.i64 = -42;
    
    int64_t val;
    mp_error_t err = mp_object_as_int(&obj, &val);
    if (err != MP_OK) append_error(test, "Failed to get int", err);
    if (val != -42) append_error(test, "Value mismatch", val);
    
    // Type checking
    obj.type = MP_TYPE_STR;
    err = mp_object_as_int(&obj, &val);
    if (err != MP_ERROR_DECODE_INVALID_FORMAT) append_error(test, "Expected type error", err);
    
    return test_end(test);
}

bool test_object_as_uint(test_result_t *test) {
    mp_object_t obj;
    obj.type = MP_TYPE_POSITIVE_INTEGER;
    obj.via.u64 = 12345;
    
    uint64_t val;
    mp_error_t err = mp_object_as_uint(&obj, &val);
    if (err != MP_OK) append_error(test, "Failed to get uint", err);
    if (val != 12345) append_error(test, "Value mismatch", val);
    
    // Coercion from positive INT
    obj.type = MP_TYPE_NEGATIVE_INTEGER;
    obj.via.i64 = 100;
    err = mp_object_as_uint(&obj, &val);
    if (err != MP_OK) append_error(test, "Failed to coerce positive int to uint", err);
    if (val != 100) append_error(test, "Value mismatch", val);
    
    // Fails on negative INT
    obj.via.i64 = -1;
    err = mp_object_as_uint(&obj, &val);
    if (err != MP_ERROR_DECODE_INVALID_FORMAT) append_error(test, "Expected type error for negative int", err);
    
    return test_end(test);
}

bool test_object_as_float(test_result_t *test) {
    mp_object_t obj;
    obj.type = MP_TYPE_FLOAT32;
    obj.via.f32 = 3.14f;
    
    float val;
    mp_error_t err = mp_object_as_float(&obj, &val);
    if (err != MP_OK) append_error(test, "Failed to get float", err);
    
    // Float coercing to Double
    double val_d;
    err = mp_object_as_double(&obj, &val_d);
    if (err != MP_OK) append_error(test, "Failed to coerce float to double", err);
    
    return test_end(test);
}

test_suite_t suite_object = {.name = "MessagePack Object Suite", .standard = "MsgPack-Object"};

int main(void) {
    r_set_global_verbosity(R_VERBOSE);
    enable_memleak_detection(&suite_object);
    
    add_test(&suite_object, test_object_as_int, "Object: Getter INT", "Object-Spec");
    add_test(&suite_object, test_object_as_uint, "Object: Getter UINT", "Object-Spec");
    add_test(&suite_object, test_object_as_float, "Object: Getter FLOAT", "Object-Spec");
    
    register_suite(&suite_object);
    bool success = run_all_suites();
    return success ? 0 : 1;
}
