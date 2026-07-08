#ifndef RADIKANT_MESSAGEPACK_OBJECT_H
#define RADIKANT_MESSAGEPACK_OBJECT_H

#include "types.h"
#include "zone.h"
#include "error.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct mp_object_t mp_object_t;

typedef struct {
    uint32_t size;
    mp_object_t* ptr;
} mp_object_array_t;

typedef struct mp_object_kv_t mp_object_kv_t;

typedef struct {
    uint32_t size;
    mp_object_kv_t* ptr;
} mp_object_map_t;

typedef struct {
    uint32_t size;
    const char* ptr;
} mp_object_str_t;

typedef struct {
    uint32_t size;
    const char* ptr;
} mp_object_bin_t;

typedef struct {
    int8_t type;
    uint32_t size;
    const char* ptr;
} mp_object_ext_t;

union mp_object_union_t {
    bool boolean;
    uint64_t u64;
    int64_t i64;
    float f32;
    double f64;
    mp_object_array_t array;
    mp_object_map_t map;
    mp_object_str_t str;
    mp_object_bin_t bin;
    mp_object_ext_t ext;
};

struct mp_object_t {
    mp_type_t type;
    union mp_object_union_t via;
};

struct mp_object_kv_t {
    mp_object_t key;
    mp_object_t val;
};

// Safe Accessors
mp_error_t mp_object_as_nil(const mp_object_t* obj);
mp_error_t mp_object_as_bool(const mp_object_t* obj, bool* out);
mp_error_t mp_object_as_int(const mp_object_t* obj, int64_t* out);
mp_error_t mp_object_as_uint(const mp_object_t* obj, uint64_t* out);
mp_error_t mp_object_as_float(const mp_object_t* obj, float* out);
mp_error_t mp_object_as_double(const mp_object_t* obj, double* out);
mp_error_t mp_object_as_str(const mp_object_t* obj, const char** out_str, uint32_t* out_len);
mp_error_t mp_object_as_bin(const mp_object_t* obj, const char** out_bin, uint32_t* out_len);
mp_error_t mp_object_as_array(const mp_object_t* obj, mp_object_t** out_elements, uint32_t* out_len);
mp_error_t mp_object_as_map(const mp_object_t* obj, mp_object_kv_t** out_elements, uint32_t* out_len);
mp_error_t mp_object_as_ext(const mp_object_t* obj, int8_t* out_type, const char** out_data, uint32_t* out_len);

#ifdef __cplusplus
}
#endif

#endif // RADIKANT_MESSAGEPACK_OBJECT_H
