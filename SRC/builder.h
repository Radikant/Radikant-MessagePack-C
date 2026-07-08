#ifndef RADIKANT_MESSAGEPACK_BUILDER_H
#define RADIKANT_MESSAGEPACK_BUILDER_H

#include "object.h"
#include "zone.h"
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

// ---------------------------------------------------------
// Primitive Builders (Inline)
// ---------------------------------------------------------
static inline void mp_build_nil(mp_object_t* obj) {
    if (obj) obj->type = MP_TYPE_NIL;
}

static inline void mp_build_bool(mp_object_t* obj, bool val) {
    if (obj) {
        obj->type = MP_TYPE_BOOLEAN;
        obj->via.boolean = val;
    }
}

static inline void mp_build_int(mp_object_t* obj, int64_t val) {
    if (obj) {
        if (val >= 0) {
            obj->type = MP_TYPE_POSITIVE_INTEGER;
            obj->via.u64 = (uint64_t)val;
        } else {
            obj->type = MP_TYPE_NEGATIVE_INTEGER;
            obj->via.i64 = val;
        }
    }
}

static inline void mp_build_uint(mp_object_t* obj, uint64_t val) {
    if (obj) {
        obj->type = MP_TYPE_POSITIVE_INTEGER;
        obj->via.u64 = val;
    }
}

static inline void mp_build_float(mp_object_t* obj, float val) {
    if (obj) {
        obj->type = MP_TYPE_FLOAT32;
        obj->via.f32 = val;
    }
}

static inline void mp_build_double(mp_object_t* obj, double val) {
    if (obj) {
        obj->type = MP_TYPE_FLOAT64;
        obj->via.f64 = val;
    }
}

// ---------------------------------------------------------
// Data References (Inline - Zero Copy)
// ---------------------------------------------------------
static inline void mp_build_str(mp_object_t* obj, const char* str, uint32_t len) {
    if (obj) {
        obj->type = MP_TYPE_STR;
        obj->via.str.ptr = str;
        obj->via.str.size = len;
    }
}

static inline void mp_build_cstr(mp_object_t* obj, const char* str) {
    if (obj && str) {
        mp_build_str(obj, str, (uint32_t)strlen(str));
    }
}

static inline void mp_build_bin(mp_object_t* obj, const char* bin, uint32_t len) {
    if (obj) {
        obj->type = MP_TYPE_BIN;
        obj->via.bin.ptr = bin;
        obj->via.bin.size = len;
    }
}

static inline void mp_build_ext(mp_object_t* obj, int8_t ext_type, const char* data, uint32_t len) {
    if (obj) {
        obj->type = MP_TYPE_EXT;
        obj->via.ext.type = ext_type;
        obj->via.ext.ptr = data;
        obj->via.ext.size = len;
    }
}

// ---------------------------------------------------------
// Collections (Allocates from zone)
// ---------------------------------------------------------
mp_error_t mp_build_array(mp_zone_t* zone, mp_object_t* obj, uint32_t size);
mp_error_t mp_build_map(mp_zone_t* zone, mp_object_t* obj, uint32_t size);

// ---------------------------------------------------------
// Collection Setters (Inline)
// ---------------------------------------------------------

static inline mp_error_t mp_array_set(mp_object_t* array_obj, uint32_t index, mp_object_t val) {
    if (!array_obj || array_obj->type != MP_TYPE_ARRAY || index >= array_obj->via.array.size) {
        return MP_ERROR_BAD_ARG;
    }
    array_obj->via.array.ptr[index] = val;
    return MP_OK;
}

static inline mp_error_t mp_map_set(mp_object_t* map_obj, uint32_t index, mp_object_t key, mp_object_t val) {
    if (!map_obj || map_obj->type != MP_TYPE_MAP || index >= map_obj->via.map.size) {
        return MP_ERROR_BAD_ARG;
    }
    map_obj->via.map.ptr[index].key = key;
    map_obj->via.map.ptr[index].val = val;
    return MP_OK;
}

// Quick setters for common map keys/values
static inline mp_error_t mp_map_set_str(mp_object_t* map_obj, uint32_t index, const char* key, const char* val) {
    mp_object_t k, v;
    mp_build_cstr(&k, key);
    mp_build_cstr(&v, val);
    return mp_map_set(map_obj, index, k, v);
}

static inline mp_error_t mp_map_set_int(mp_object_t* map_obj, uint32_t index, const char* key, int64_t val) {
    mp_object_t k, v;
    mp_build_cstr(&k, key);
    mp_build_int(&v, val);
    return mp_map_set(map_obj, index, k, v);
}

static inline mp_error_t mp_map_set_bool(mp_object_t* map_obj, uint32_t index, const char* key, bool val) {
    mp_object_t k, v;
    mp_build_cstr(&k, key);
    mp_build_bool(&v, val);
    return mp_map_set(map_obj, index, k, v);
}

static inline mp_error_t mp_map_set_double(mp_object_t* map_obj, uint32_t index, const char* key, double val) {
    mp_object_t k, v;
    mp_build_cstr(&k, key);
    mp_build_double(&v, val);
    return mp_map_set(map_obj, index, k, v);
}


#ifdef __cplusplus
}
#endif

#endif // RADIKANT_MESSAGEPACK_BUILDER_H
