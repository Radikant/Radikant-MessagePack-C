#ifndef RADIKANT_MESSAGEPACK_ERROR_H
#define RADIKANT_MESSAGEPACK_ERROR_H

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
  MP_OK = 0,
  
  // General / Argument Errors
  MP_ERROR_BAD_ARG = -1, // Null pointer passed to API
  MP_ERROR_NOMEM = -2,   // Memory allocation failed (zone_alloc)

  // Decoder Specific Errors
  MP_ERROR_DECODE_INCOMPLETE        = -10, // Stream ended unexpectedly (general)
  MP_ERROR_DECODE_TRUNCATED_STR     = -11, // Stream ended while reading string
  MP_ERROR_DECODE_TRUNCATED_BIN     = -12, // Stream ended while reading binary
  MP_ERROR_DECODE_TRUNCATED_EXT     = -13, // Stream ended while reading extension
  MP_ERROR_DECODE_TRUNCATED_ARRAY   = -14, // Stream ended while reading array elements
  MP_ERROR_DECODE_TRUNCATED_MAP     = -15, // Stream ended while reading map key/values
  MP_ERROR_DECODE_INVALID_FORMAT    = -16, // Unknown or illegal tag byte encountered
  MP_ERROR_DECODE_UNSUPPORTED_TYPE  = -17, // Valid tag, but this library doesn't support parsing it yet
  MP_ERROR_DECODE_DEPTH_EXCEEDED    = -18, // Recursion too deep (arrays/maps)

  // Encoder Specific Errors
  MP_ERROR_ENCODE_BUFFER_OVERFLOW   = -30, // Buffer size exceeded maximum limits
  MP_ERROR_ENCODE_UNSUPPORTED_TYPE  = -31, // Attempted to encode an invalid or unsupported type
  MP_ERROR_ENCODE_NOMEM             = -32, // Failed to grow encoder buffer
  MP_ERROR_ENCODE_NULL_PAYLOAD      = -33, // Attempted to encode a string/bin with a NULL data pointer
  MP_ERROR_ENCODE_INVALID_EXT_TYPE  = -34, // Provided extension type integer is out of protocol bounds
  MP_ERROR_ENCODE_MAP_KEY_TYPE      = -35, // Strict mode: Map key is not a valid type (e.g., non-string)
  MP_ERROR_ENCODE_DEPTH_EXCEEDED    = -36, // Object tree recursion too deep during encoding
  MP_ERROR_ENCODE_FLOAT_NAN         = -37, // Attempted to encode a NaN/Inf float (if strict mode enabled)

  // Stream Specific Errors
  MP_ERROR_STREAM_BAD_ARG           = -40, // Passed a NULL stream or buffer to init
  MP_ERROR_STREAM_NOT_INITIALIZED   = -41, // Tried to read/write but stream is missing read/write function pointers
  MP_ERROR_STREAM_OPEN_FAIL         = -42, // Failed to open a file/socket stream
  MP_ERROR_STREAM_UNSUPPORTED       = -43, // Stream does not support requested operation (e.g., mark/reset on a live socket)

  // Object / Builder Specific Errors
  MP_ERROR_OBJECT_TYPE_MISMATCH     = -50, // Attempted to read an object as a type it is not
  MP_ERROR_OBJECT_OUT_OF_BOUNDS     = -51, // Attempted to access an array/map index outside its size
  MP_ERROR_OBJECT_NULL_POINTER      = -52, // Passed a NULL object pointer to an accessor or builder

  // Query Specific Errors
  MP_ERROR_QUERY_NOT_FOUND          = -60, // The requested key or index was not found in the collection
  MP_ERROR_QUERY_TYPE_MISMATCH      = -61, // Tried to search a Map by key but hit an Array (or vice versa)
  MP_ERROR_QUERY_MAX_DEPTH          = -62  // The query path exceeded MP_MAX_QUERY_DEPTH limits
} mp_error_t;

#ifdef __cplusplus
}
#endif

#endif // RADIKANT_MESSAGEPACK_ERROR_H
