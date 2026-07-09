#include "query.h"
#include "skip.h"
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
