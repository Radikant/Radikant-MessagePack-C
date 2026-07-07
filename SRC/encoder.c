#include "encoder.h"
#include <stdlib.h>
#include <string.h>

static inline void serialize_be16(char* buf, uint16_t v) {
    buf[0] = (char)(v >> 8); buf[1] = (char)v;
}
static inline void serialize_be32(char* buf, uint32_t v) {
    buf[0] = (char)(v >> 24); buf[1] = (char)(v >> 16); buf[2] = (char)(v >> 8); buf[3] = (char)v;
}
static inline void serialize_be64(char* buf, uint64_t v) {
    buf[0] = (char)(v >> 56); buf[1] = (char)(v >> 48); buf[2] = (char)(v >> 40); buf[3] = (char)(v >> 32);
    buf[4] = (char)(v >> 24); buf[5] = (char)(v >> 16); buf[6] = (char)(v >> 8); buf[7] = (char)v;
}

static inline mp_error_t mp_encode_bytes(mp_stream_t* stream, const void* data, size_t len) {
    if (!stream || !stream->write) return MP_ERROR_BAD_ARG;
    return stream->write(stream, data, len);
}

mp_error_t mp_encode_nil(mp_stream_t* stream) {
    char d = (char)MP_TAG_NIL;
    return mp_encode_bytes(stream, &d, sizeof(d));
}

mp_error_t mp_encode_bool(mp_stream_t* stream, bool val) {
    char d = val ? (char)MP_TAG_TRUE : (char)MP_TAG_FALSE;
    return mp_encode_bytes(stream, &d, sizeof(d));
}

mp_error_t mp_encode_int(mp_stream_t* stream, int64_t val) {
    if (val >= -32 && val <= 127) {
        char d = (char)val;
        return mp_encode_bytes(stream, &d, sizeof(d));
    } else if (val >= -128 && val < 0) {
        char d[2] = { (char)MP_TAG_INT8, (char)val };
        return mp_encode_bytes(stream, d, sizeof(d));
    } else if (val >= -32768 && val < 0) {
        char d[3] = { (char)MP_TAG_INT16 };
        serialize_be16(d+1, (uint16_t)val);
        return mp_encode_bytes(stream, d, sizeof(d));
    } else if (val >= -2147483648LL && val < 0) {
        char d[5] = { (char)MP_TAG_INT32 };
        serialize_be32(d+1, (uint32_t)val);
        return mp_encode_bytes(stream, d, sizeof(d));
    } else {
        char d[9] = { (char)MP_TAG_INT64 };
        serialize_be64(d+1, (uint64_t)val);
        return mp_encode_bytes(stream, d, sizeof(d));
    }
}

mp_error_t mp_encode_uint(mp_stream_t* stream, uint64_t val) {
    if (val <= 127) {
        char d = (char)val;
        return mp_encode_bytes(stream, &d, sizeof(d));
    } else if (val <= 255) {
        char d[2] = { (char)MP_TAG_UINT8, (char)val };
        return mp_encode_bytes(stream, d, sizeof(d));
    } else if (val <= 65535) {
        char d[3] = { (char)MP_TAG_UINT16 };
        serialize_be16(d+1, (uint16_t)val);
        return mp_encode_bytes(stream, d, sizeof(d));
    } else if (val <= 4294967295ULL) {
        char d[5] = { (char)MP_TAG_UINT32 };
        serialize_be32(d+1, (uint32_t)val);
        return mp_encode_bytes(stream, d, sizeof(d));
    } else {
        char d[9] = { (char)MP_TAG_UINT64 };
        serialize_be64(d+1, val);
        return mp_encode_bytes(stream, d, sizeof(d));
    }
}

mp_error_t mp_encode_float(mp_stream_t* stream, float val) {
    union { float f; uint32_t u; } u = {val};
    char d[5] = { (char)MP_TAG_FLOAT32 };
    serialize_be32(d+1, u.u);
    return mp_encode_bytes(stream, d, sizeof(d));
}

mp_error_t mp_encode_double(mp_stream_t* stream, double val) {
    union { double d; uint64_t u; } u = {val};
    char d[9] = { (char)MP_TAG_FLOAT64 };
    serialize_be64(d+1, u.u);
    return mp_encode_bytes(stream, d, sizeof(d));
}

mp_error_t mp_encode_str_len(mp_stream_t* stream, uint32_t len) {
    if (len <= 31) {
        char d = (char)(MP_TAG_FIXSTR_MIN | len);
        return mp_encode_bytes(stream, &d, sizeof(d));
    } else if (len <= 255) {
        char d[2] = { (char)MP_TAG_STR8, (char)len };
        return mp_encode_bytes(stream, d, sizeof(d));
    } else if (len <= 65535) {
        char d[3] = { (char)MP_TAG_STR16 };
        serialize_be16(d+1, (uint16_t)len);
        return mp_encode_bytes(stream, d, sizeof(d));
    } else {
        char d[5] = { (char)MP_TAG_STR32 };
        serialize_be32(d+1, len);
        return mp_encode_bytes(stream, d, sizeof(d));
    }
}

mp_error_t mp_encode_str(mp_stream_t* stream, const char* str, uint32_t len) {
    if (!str && len > 0) return MP_ERROR_ENCODE_NULL_PAYLOAD;
    mp_error_t err = mp_encode_str_len(stream, len);
    if (err != MP_OK) return err;
    if (len > 0) return mp_encode_bytes(stream, str, len);
    return MP_OK;
}

mp_error_t mp_encode_bin_len(mp_stream_t* stream, uint32_t len) {
    if (len <= 255) {
        char d[2] = { (char)MP_TAG_BIN8, (char)len };
        return mp_encode_bytes(stream, d, sizeof(d));
    } else if (len <= 65535) {
        char d[3] = { (char)MP_TAG_BIN16 };
        serialize_be16(d+1, (uint16_t)len);
        return mp_encode_bytes(stream, d, sizeof(d));
    } else {
        char d[5] = { (char)MP_TAG_BIN32 };
        serialize_be32(d+1, len);
        return mp_encode_bytes(stream, d, sizeof(d));
    }
}

mp_error_t mp_encode_bin(mp_stream_t* stream, const char* bin, uint32_t len) {
    if (!bin && len > 0) return MP_ERROR_ENCODE_NULL_PAYLOAD;
    mp_error_t err = mp_encode_bin_len(stream, len);
    if (err != MP_OK) return err;
    if (len > 0) return mp_encode_bytes(stream, bin, len);
    return MP_OK;
}

mp_error_t mp_encode_array_len(mp_stream_t* stream, uint32_t len) {
    if (len <= 15) {
        char d = (char)(MP_TAG_FIXARRAY_MIN | len);
        return mp_encode_bytes(stream, &d, sizeof(d));
    } else if (len <= 65535) {
        char d[3] = { (char)MP_TAG_ARRAY16 };
        serialize_be16(d+1, (uint16_t)len);
        return mp_encode_bytes(stream, d, sizeof(d));
    } else {
        char d[5] = { (char)MP_TAG_ARRAY32 };
        serialize_be32(d+1, len);
        return mp_encode_bytes(stream, d, sizeof(d));
    }
}

mp_error_t mp_encode_map_len(mp_stream_t* stream, uint32_t len) {
    if (len <= 15) {
        char d = (char)(MP_TAG_FIXMAP_MIN | len);
        return mp_encode_bytes(stream, &d, sizeof(d));
    } else if (len <= 65535) {
        char d[3] = { (char)MP_TAG_MAP16 };
        serialize_be16(d+1, (uint16_t)len);
        return mp_encode_bytes(stream, d, sizeof(d));
    } else {
        char d[5] = { (char)MP_TAG_MAP32 };
        serialize_be32(d+1, len);
        return mp_encode_bytes(stream, d, sizeof(d));
    }
}
