#ifndef RADIKANT_MESSAGEPACK_DECODER_H
#define RADIKANT_MESSAGEPACK_DECODER_H

#include "types.h"
#include "error.h"
#include "zone.h"
#include "object.h"
#include "stream.h"
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    mp_stream_t* stream;
    mp_zone_t* zone;
    uint32_t depth;
    uint32_t max_depth;
} mp_decoder_t;

void mp_decoder_init(mp_decoder_t* decoder, mp_stream_t* stream, mp_zone_t* zone);
mp_error_t mp_decode(mp_decoder_t* decoder, mp_object_t* out_obj);

// Parses a complete MessagePack payload from a memory buffer into the given root object.
// Memory for strings, bins, and arrays/maps will be allocated from `zone`.
mp_error_t mp_parse_memory(mp_zone_t* zone, const char* data, size_t size, mp_object_t* out_obj);

// -------------------------------------------------------------------------
// Incremental Streaming Decoders (SAX API)
// -------------------------------------------------------------------------
// These functions read primitives from the stream directly. They do not
// perform memory allocations or build an AST.

mp_error_t mp_decode_stream_nil(mp_stream_t* stream);
mp_error_t mp_decode_stream_bool(mp_stream_t* stream, bool* out);
mp_error_t mp_decode_stream_int(mp_stream_t* stream, int64_t* out);
mp_error_t mp_decode_stream_uint(mp_stream_t* stream, uint64_t* out);
mp_error_t mp_decode_stream_float(mp_stream_t* stream, float* out);
mp_error_t mp_decode_stream_double(mp_stream_t* stream, double* out);

// Reads the string marker and length. Does NOT read the string body.
// The user must subsequently call stream->read() to fetch `out_len` bytes.
mp_error_t mp_decode_stream_str_len(mp_stream_t* stream, uint32_t* out_len);

// Reads the binary marker and length. Does NOT read the binary body.
// The user must subsequently call stream->read() to fetch `out_len` bytes.
mp_error_t mp_decode_stream_bin_len(mp_stream_t* stream, uint32_t* out_len);

// Reads the array marker and length.
// The user must subsequently parse `out_len` elements from the stream.
mp_error_t mp_decode_stream_array_len(mp_stream_t* stream, uint32_t* out_len);

// Reads the map marker and length.
// The user must subsequently parse `out_len * 2` elements from the stream.
mp_error_t mp_decode_stream_map_len(mp_stream_t* stream, uint32_t* out_len);

// Reads the extension marker, length, and type. Does NOT read the extension body.
// The user must subsequently call stream->read() to fetch `out_len` bytes.
mp_error_t mp_decode_stream_ext_len(mp_stream_t* stream, int8_t* out_type, uint32_t* out_len);

#ifdef __cplusplus
}
#endif

#endif // RADIKANT_MESSAGEPACK_DECODER_H
