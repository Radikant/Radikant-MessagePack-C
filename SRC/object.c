#include "object.h"
#include "error.h"

mp_error_t mp_object_as_nil(const mp_object_t* obj) {
    if (!obj) return MP_ERROR_OBJECT_NULL_POINTER;
    if (obj->type != MP_TYPE_NIL) return MP_ERROR_OBJECT_TYPE_MISMATCH; // Or type mismatch
    return MP_OK;
}

mp_error_t mp_object_as_bool(const mp_object_t* obj, bool* out) {
    if (!obj) return MP_ERROR_OBJECT_NULL_POINTER;
    if (!out) return MP_ERROR_BAD_ARG;
    if (obj->type != MP_TYPE_BOOLEAN) return MP_ERROR_OBJECT_TYPE_MISMATCH;
    *out = obj->via.boolean;
    return MP_OK;
}

mp_error_t mp_object_as_int(const mp_object_t* obj, int64_t* out) {
    if (!obj) return MP_ERROR_OBJECT_NULL_POINTER;
    if (!out) return MP_ERROR_BAD_ARG;
    if (obj->type == MP_TYPE_POSITIVE_INTEGER) {
        *out = (int64_t)obj->via.u64;
        return MP_OK;
    } else if (obj->type == MP_TYPE_NEGATIVE_INTEGER) {
        *out = obj->via.i64;
        return MP_OK;
    }
    return MP_ERROR_OBJECT_TYPE_MISMATCH;
}

mp_error_t mp_object_as_uint(const mp_object_t* obj, uint64_t* out) {
    if (!obj) return MP_ERROR_OBJECT_NULL_POINTER;
    if (!out) return MP_ERROR_BAD_ARG;
    if (obj->type == MP_TYPE_POSITIVE_INTEGER) {
        *out = obj->via.u64;
        return MP_OK;
    } else if (obj->type == MP_TYPE_NEGATIVE_INTEGER && obj->via.i64 >= 0) {
        *out = (uint64_t)obj->via.i64;
        return MP_OK;
    }
    return MP_ERROR_OBJECT_TYPE_MISMATCH;
}

mp_error_t mp_object_as_float(const mp_object_t* obj, float* out) {
    if (!obj) return MP_ERROR_OBJECT_NULL_POINTER;
    if (!out) return MP_ERROR_BAD_ARG;
    if (obj->type == MP_TYPE_FLOAT32) {
        *out = obj->via.f32;
        return MP_OK;
    } else if (obj->type == MP_TYPE_FLOAT64) {
        *out = (float)obj->via.f64;
        return MP_OK;
    }
    return MP_ERROR_OBJECT_TYPE_MISMATCH;
}

mp_error_t mp_object_as_double(const mp_object_t* obj, double* out) {
    if (!obj) return MP_ERROR_OBJECT_NULL_POINTER;
    if (!out) return MP_ERROR_BAD_ARG;
    if (obj->type == MP_TYPE_FLOAT64) {
        *out = obj->via.f64;
        return MP_OK;
    } else if (obj->type == MP_TYPE_FLOAT32) {
        *out = (double)obj->via.f32;
        return MP_OK;
    }
    return MP_ERROR_OBJECT_TYPE_MISMATCH;
}

mp_error_t mp_object_as_str(const mp_object_t* obj, const char** out_str, uint32_t* out_len) {
    if (!obj) return MP_ERROR_OBJECT_NULL_POINTER;
    if (!out_str || !out_len) return MP_ERROR_BAD_ARG;
    if (obj->type != MP_TYPE_STR) return MP_ERROR_OBJECT_TYPE_MISMATCH;
    *out_str = obj->via.str.ptr;
    *out_len = obj->via.str.size;
    return MP_OK;
}

mp_error_t mp_object_as_bin(const mp_object_t* obj, const char** out_bin, uint32_t* out_len) {
    if (!obj) return MP_ERROR_OBJECT_NULL_POINTER;
    if (!out_bin || !out_len) return MP_ERROR_BAD_ARG;
    if (obj->type != MP_TYPE_BIN) return MP_ERROR_OBJECT_TYPE_MISMATCH;
    *out_bin = obj->via.bin.ptr;
    *out_len = obj->via.bin.size;
    return MP_OK;
}

mp_error_t mp_object_as_array(const mp_object_t* obj, mp_object_t** out_elements, uint32_t* out_len) {
    if (!obj) return MP_ERROR_OBJECT_NULL_POINTER;
    if (!out_elements || !out_len) return MP_ERROR_BAD_ARG;
    if (obj->type != MP_TYPE_ARRAY) return MP_ERROR_OBJECT_TYPE_MISMATCH;
    *out_elements = obj->via.array.ptr;
    *out_len = obj->via.array.size;
    return MP_OK;
}

mp_error_t mp_object_as_map(const mp_object_t* obj, mp_object_kv_t** out_elements, uint32_t* out_len) {
    if (!obj) return MP_ERROR_OBJECT_NULL_POINTER;
    if (!out_elements || !out_len) return MP_ERROR_BAD_ARG;
    if (obj->type != MP_TYPE_MAP) return MP_ERROR_OBJECT_TYPE_MISMATCH;
    *out_elements = obj->via.map.ptr;
    *out_len = obj->via.map.size;
    return MP_OK;
}

mp_error_t mp_object_as_ext(const mp_object_t* obj, int8_t* out_type, const char** out_data, uint32_t* out_len) {
    if (!obj) return MP_ERROR_OBJECT_NULL_POINTER;
    if (!out_type || !out_data || !out_len) return MP_ERROR_BAD_ARG;
    if (obj->type != MP_TYPE_EXT) return MP_ERROR_OBJECT_TYPE_MISMATCH;
    *out_type = obj->via.ext.type;
    *out_data = obj->via.ext.ptr;
    *out_len = obj->via.ext.size;
    return MP_OK;
}
