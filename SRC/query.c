#include "query.h"
#include "skip.h"
#include "builder.h"
#include <string.h>

mp_error_t mp_query_map_key_str(mp_decoder_t* decoder, const char* search_key) {
    if (!decoder || !search_key) return MP_ERROR_BAD_ARG;

    uint32_t map_len;
    mp_error_t err = mp_decode_stream_map_len(decoder->stream, &map_len);
    if (err != MP_OK) return err;

    size_t target_len = strlen(search_key);

    for (uint32_t i = 0; i < map_len; i++) {
        uint8_t tag;
        err = decoder->stream->read(decoder->stream, &tag, 1);
        if (err != MP_OK) return err;

        uint32_t key_len = 0;
        bool is_string = false;

        if (tag >= MP_TAG_FIXSTR_MIN && tag <= MP_TAG_FIXSTR_MAX) {
            key_len = tag & 0x1f;
            is_string = true;
        } else if (tag == MP_TAG_STR8) {
            uint8_t len8;
            err = decoder->stream->read(decoder->stream, &len8, 1);
            if (err != MP_OK) return err;
            key_len = len8;
            is_string = true;
        } else if (tag == MP_TAG_STR16) {
            char len16[2];
            err = decoder->stream->read(decoder->stream, len16, 2);
            if (err != MP_OK) return err;
            key_len = ((uint32_t)(uint8_t)len16[0] << 8) | (uint8_t)len16[1];
            is_string = true;
        } else if (tag == MP_TAG_STR32) {
            char len32[4];
            err = decoder->stream->read(decoder->stream, len32, 4);
            if (err != MP_OK) return err;
            key_len = ((uint32_t)(uint8_t)len32[0] << 24) | ((uint32_t)(uint8_t)len32[1] << 16) |
                      ((uint32_t)(uint8_t)len32[2] << 8) | (uint8_t)len32[3];
            is_string = true;
        }

        if (is_string) {
            if (key_len == target_len) {
                // Potential match, read and compare safely
                char buf[256];
                if (key_len <= sizeof(buf)) {
                    err = decoder->stream->read(decoder->stream, buf, key_len);
                    if (err != MP_OK) return err;
                    
                    if (memcmp(buf, search_key, key_len) == 0) {
                        return MP_OK; // Match found! Decoder is positioned at the value.
                    }
                } else {
                    err = decoder->stream->skip(decoder->stream, key_len);
                    if (err != MP_OK) return err;
                }
            } else {
                // Length mismatch, just skip the string body
                err = decoder->stream->skip(decoder->stream, key_len);
                if (err != MP_OK) return err;
            }
        } else {
            // It's not a string key, we must skip it using the tag we already consumed
            err = mp_skip_tag(decoder, tag);
            if (err != MP_OK) return err;
        }

        // We didn't match the key, so we must skip the value to get to the next pair
        err = mp_skip(decoder);
        if (err != MP_OK) return err;
    }

    return MP_ERROR_QUERY_NOT_FOUND;
}

mp_error_t mp_query_array_index(mp_decoder_t* decoder, uint32_t target_index) {
    if (!decoder) return MP_ERROR_BAD_ARG;

    uint32_t array_len;
    mp_error_t err = mp_decode_stream_array_len(decoder->stream, &array_len);
    if (err != MP_OK) return err;

    if (target_index >= array_len) {
        // Index out of bounds, consume the whole array to leave stream cleanly positioned after it
        for (uint32_t i = 0; i < array_len; i++) {
            err = mp_skip(decoder);
            if (err != MP_OK) return err;
        }
        return MP_ERROR_QUERY_NOT_FOUND;
    }

    // Skip elements until we reach the target index
    for (uint32_t i = 0; i < target_index; i++) {
        err = mp_skip(decoder);
        if (err != MP_OK) return err;
    }

    return MP_OK; // Decoder is perfectly positioned at the requested index
}

// -----------------------------------------------------------------------------
// Query Path API (Concept 1: Deep Plucking)
// -----------------------------------------------------------------------------

void mp_query_init(mp_query_t* q) {
    if (q) q->count = 0;
}

mp_error_t mp_query_add_path_str(mp_query_t* q, const char* key) {
    if (!q || !key) return MP_ERROR_BAD_ARG;
    if (q->count >= MP_MAX_QUERY_DEPTH) return MP_ERROR_BAD_ARG; // Max depth reached
    q->steps[q->count].type = MP_QUERY_PATH_STR;
    q->steps[q->count].via.str = key;
    q->count++;
    return MP_OK;
}

mp_error_t mp_query_add_path_idx(mp_query_t* q, uint32_t index) {
    if (!q) return MP_ERROR_BAD_ARG;
    if (q->count >= MP_MAX_QUERY_DEPTH) return MP_ERROR_BAD_ARG; // Max depth reached
    q->steps[q->count].type = MP_QUERY_PATH_INDEX;
    q->steps[q->count].via.index = index;
    q->count++;
    return MP_OK;
}

mp_error_t mp_query_execute(mp_decoder_t* decoder, const mp_query_t* q) {
    if (!decoder || !q) return MP_ERROR_BAD_ARG;

    for (uint32_t i = 0; i < q->count; i++) {
        mp_error_t err;
        if (q->steps[i].type == MP_QUERY_PATH_STR) {
            err = mp_query_map_key_str(decoder, q->steps[i].via.str);
        } else {
            err = mp_query_array_index(decoder, q->steps[i].via.index);
        }
        
        if (err != MP_OK) {
            return err; // Query path failed at this step
        }
    }

    return MP_OK;
}

// -----------------------------------------------------------------------------
// Field Extraction API (Concept 2: Shallow Flattening)
// -----------------------------------------------------------------------------

mp_error_t mp_query_extract_fields(
    mp_decoder_t* decoder, 
    mp_zone_t* zone, 
    const char** fields, 
    uint32_t field_count, 
    mp_object_t* out_map) 
{
    if (!decoder || !zone || !fields || !out_map) return MP_ERROR_BAD_ARG;

    uint32_t map_len;
    mp_error_t err = mp_decode_stream_map_len(decoder->stream, &map_len);
    if (err != MP_OK) return err; // If it's not a map, this will correctly fail

    // Attach zone temporarily if decoder doesn't have one, or just use the passed zone directly
    mp_zone_t* old_zone = decoder->zone;
    decoder->zone = zone;

    // We pre-allocate the maximum possible size for the AST map (field_count)
    // If the stream map has fewer matching fields, we will shrink the size later.
    // The memory overhead is harmless because it's arena-allocated.
    err = mp_build_map(zone, out_map, field_count);
    if (err != MP_OK) {
        decoder->zone = old_zone;
        return err;
    }

    uint32_t extracted = 0;

    for (uint32_t i = 0; i < map_len; i++) {
        uint8_t tag;
        err = decoder->stream->read(decoder->stream, &tag, 1);
        if (err != MP_OK) goto cleanup;

        uint32_t key_len = 0;
        bool is_string = false;

        // Check if the key is a string and determine its length
        if (tag >= MP_TAG_FIXSTR_MIN && tag <= MP_TAG_FIXSTR_MAX) {
            key_len = tag & 0x1f;
            is_string = true;
        } else if (tag == MP_TAG_STR8) {
            uint8_t len8;
            err = decoder->stream->read(decoder->stream, &len8, 1);
            if (err != MP_OK) goto cleanup;
            key_len = len8;
            is_string = true;
        } else if (tag == MP_TAG_STR16) {
            char len16[2];
            err = decoder->stream->read(decoder->stream, len16, 2);
            if (err != MP_OK) goto cleanup;
            key_len = ((uint32_t)(uint8_t)len16[0] << 8) | (uint8_t)len16[1];
            is_string = true;
        } else if (tag == MP_TAG_STR32) {
            char len32[4];
            err = decoder->stream->read(decoder->stream, len32, 4);
            if (err != MP_OK) goto cleanup;
            key_len = ((uint32_t)(uint8_t)len32[0] << 24) | ((uint32_t)(uint8_t)len32[1] << 16) |
                      ((uint32_t)(uint8_t)len32[2] << 8) | (uint8_t)len32[3];
            is_string = true;
        }

        bool matched = false;

        if (is_string) {
            // Read the key string into a stack buffer for comparison
            char buf[256];
            char* key_ptr = buf;
            bool alloc_key = false;

            if (key_len >= sizeof(buf)) {
                // If it's huge, we dynamically allocate it to the zone right away
                key_ptr = (char*)mp_zone_alloc(zone, key_len);
                if (!key_ptr) {
                    err = MP_ERROR_NOMEM;
                    goto cleanup;
                }
                alloc_key = true;
            }

            err = decoder->stream->read(decoder->stream, key_ptr, key_len);
            if (err != MP_OK) goto cleanup;

            // Check against requested fields
            for (uint32_t j = 0; j < field_count; j++) {
                if (strlen(fields[j]) == key_len && memcmp(fields[j], key_ptr, key_len) == 0) {
                    matched = true;
                    break;
                }
            }

            if (matched) {
                if (extracted >= field_count) {
                    // Prevent buffer overflow if the stream map has duplicate keys
                    err = MP_ERROR_DECODE_INVALID_FORMAT;
                    goto cleanup;
                }

                // If we used the stack buffer, we now need to copy it to the zone
                if (!alloc_key) {
                    char* final_key_ptr = (char*)mp_zone_alloc(zone, key_len + 1);
                    if (!final_key_ptr) {
                        err = MP_ERROR_NOMEM;
                        goto cleanup;
                    }
                    memcpy(final_key_ptr, key_ptr, key_len);
                    final_key_ptr[key_len] = '\0'; // Optional null-terminator for convenience
                    key_ptr = final_key_ptr;
                }

                // Attach key to AST
                out_map->via.map.ptr[extracted].key.type = MP_TYPE_STR;
                out_map->via.map.ptr[extracted].key.via.str.ptr = key_ptr;
                out_map->via.map.ptr[extracted].key.via.str.size = key_len;

                // Decode value into AST
                err = mp_decode(decoder, &out_map->via.map.ptr[extracted].val);
                if (err != MP_OK) goto cleanup;

                extracted++;
            }
        } else {
            // Not a string key, skip it
            err = mp_skip_tag(decoder, tag);
            if (err != MP_OK) goto cleanup;
        }

        if (!matched) {
            // We read or skipped the key, but it didn't match. Skip the value.
            err = mp_skip(decoder);
            if (err != MP_OK) goto cleanup;
        }
    }

    // Shrink the map size to exactly how many fields we successfully extracted
    out_map->via.map.size = extracted;

cleanup:
    decoder->zone = old_zone;
    return err;
}
