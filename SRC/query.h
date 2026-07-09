#ifndef RADIKANT_MESSAGEPACK_QUERY_H
#define RADIKANT_MESSAGEPACK_QUERY_H

#include "decoder.h"

#ifdef __cplusplus
extern "C" {
#endif

// Searches the current map for a specific string key.
// The decoder MUST be positioned at the start of a map.
// If the key is found, the function returns MP_OK and leaves the stream positioned exactly at the value.
// If the key is NOT found, it returns MP_ERROR_QUERY_NOT_FOUND and the stream is positioned after the map.
// Note: This operation consumes the stream!
mp_error_t mp_query_map_key_str(mp_decoder_t* decoder, const char* search_key);

// Skips elements in the current array to reach a specific index.
// The decoder MUST be positioned at the start of an array.
// If the index is valid, it returns MP_OK and leaves the stream positioned exactly at the element.
// If the index is out of bounds, it returns MP_ERROR_QUERY_NOT_FOUND and the stream is positioned after the array.
// Note: This operation consumes the stream!
mp_error_t mp_query_array_index(mp_decoder_t* decoder, uint32_t target_index);

#ifdef __cplusplus
}
#endif

#endif // RADIKANT_MESSAGEPACK_QUERY_H
