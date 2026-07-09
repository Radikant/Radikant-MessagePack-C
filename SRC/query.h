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

// -----------------------------------------------------------------------------
// Query Path API (Concept 1: Deep Plucking)
// -----------------------------------------------------------------------------

#define MP_MAX_QUERY_DEPTH 16

typedef enum {
    MP_QUERY_PATH_STR,
    MP_QUERY_PATH_INDEX
} mp_query_path_type_t;

typedef struct {
    mp_query_path_type_t type;
    union {
        const char* str;
        uint32_t index;
    } via;
} mp_query_path_step_t;

typedef struct {
    mp_query_path_step_t steps[MP_MAX_QUERY_DEPTH];
    uint32_t count;
} mp_query_t;

void mp_query_init(mp_query_t* q);
mp_error_t mp_query_add_path_str(mp_query_t* q, const char* key);
mp_error_t mp_query_add_path_idx(mp_query_t* q, uint32_t index);

mp_error_t mp_query_execute(mp_decoder_t* decoder, const mp_query_t* q);

// -----------------------------------------------------------------------------
// Field Extraction API (Concept 2: Shallow Flattening)
// -----------------------------------------------------------------------------

// Extracts specific fields from the current map in the stream into the out_map.
// Requires a valid zone for AST allocation.
mp_error_t mp_query_extract_fields(
    mp_decoder_t* decoder, 
    mp_zone_t* zone, 
    const char** fields, 
    uint32_t field_count, 
    mp_object_t* out_map
);

#ifdef __cplusplus
}
#endif

#endif // RADIKANT_MESSAGEPACK_QUERY_H
