#ifndef RADIKANT_MESSAGEPACK_ENCODER_H
#define RADIKANT_MESSAGEPACK_ENCODER_H

#include "types.h"
#include "error.h"
#include "stream.h"
#include "object.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// Primitive Encoders
mp_error_t mp_encode_nil(mp_stream_t* stream);
mp_error_t mp_encode_bool(mp_stream_t* stream, bool val);
mp_error_t mp_encode_int(mp_stream_t* stream, int64_t val);
mp_error_t mp_encode_uint(mp_stream_t* stream, uint64_t val);
mp_error_t mp_encode_float(mp_stream_t* stream, float val);
mp_error_t mp_encode_double(mp_stream_t* stream, double val);

// String Encoders
mp_error_t mp_encode_str_len(mp_stream_t* stream, uint32_t len);
mp_error_t mp_encode_str(mp_stream_t* stream, const char* str, uint32_t len);

// Binary Encoders
mp_error_t mp_encode_bin_len(mp_stream_t* stream, uint32_t len);
mp_error_t mp_encode_bin(mp_stream_t* stream, const char* bin, uint32_t len);

// Array and Map Encoders
mp_error_t mp_encode_array_len(mp_stream_t* stream, uint32_t len);
mp_error_t mp_encode_map_len(mp_stream_t* stream, uint32_t len);

// Extension Encoders
mp_error_t mp_encode_ext_len(mp_stream_t* stream, int8_t type, uint32_t len);
mp_error_t mp_encode_ext(mp_stream_t* stream, int8_t type, const char* data, uint32_t len);

// Generic Object Encoder
mp_error_t mp_encode_object(mp_stream_t* stream, const mp_object_t* obj);

#ifdef __cplusplus
}
#endif

#endif // RADIKANT_MESSAGEPACK_ENCODER_H
