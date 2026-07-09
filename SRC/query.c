#include "query.h"
#include "builder.h"
#include "skip.h"
#include <string.h>

mp_error_t mp_query_map_key_str(mp_decoder_t *decoder, const char *search_key) {
  if (!decoder || !search_key)
    return MP_ERROR_BAD_ARG;

  uint32_t map_len;
  mp_error_t err = mp_decode_stream_map_len(decoder->stream, &map_len);
  if (err == MP_ERROR_DECODE_INVALID_FORMAT) {
      return MP_ERROR_QUERY_TYPE_MISMATCH;
  } else if (err != MP_OK) {
      return err;
  }

  size_t target_len = strlen(search_key);

  for (uint32_t i = 0; i < map_len; i++) {
    uint8_t tag;
    err = decoder->stream->read(decoder->stream, &tag, 1);
    if (err != MP_OK)
      return err;

    uint32_t key_len = 0;
    bool is_string = false;

    if (tag >= MP_TAG_FIXSTR_MIN && tag <= MP_TAG_FIXSTR_MAX) {
      key_len = tag & 0x1f;
      is_string = true;
    } else if (tag == MP_TAG_STR8) {
      uint8_t len8;
      err = decoder->stream->read(decoder->stream, &len8, 1);
      if (err != MP_OK)
        return err;
      key_len = len8;
      is_string = true;
    } else if (tag == MP_TAG_STR16) {
      char len16[2];
      err = decoder->stream->read(decoder->stream, len16, 2);
      if (err != MP_OK)
        return err;
      key_len = ((uint32_t)(uint8_t)len16[0] << 8) | (uint8_t)len16[1];
      is_string = true;
    } else if (tag == MP_TAG_STR32) {
      char len32[4];
      err = decoder->stream->read(decoder->stream, len32, 4);
      if (err != MP_OK)
        return err;
      key_len = ((uint32_t)(uint8_t)len32[0] << 24) |
                ((uint32_t)(uint8_t)len32[1] << 16) |
                ((uint32_t)(uint8_t)len32[2] << 8) | (uint8_t)len32[3];
      is_string = true;
    }

    if (is_string) {
      if (key_len == target_len) {
        // Potential match, read and compare safely
        char buf[256];
        if (key_len <= sizeof(buf)) {
          err = decoder->stream->read(decoder->stream, buf, key_len);
          if (err != MP_OK)
            return err;

          if (memcmp(buf, search_key, key_len) == 0) {
            return MP_OK; // Match found! Decoder is positioned at the value.
          }
        } else {
          // Compare in chunks to avoid large stack allocation
          uint32_t remaining = key_len;
          const char *cmp_ptr = search_key;
          bool matches = true;
          while (remaining > 0) {
            uint32_t chunk = remaining < sizeof(buf) ? remaining : sizeof(buf);
            err = decoder->stream->read(decoder->stream, buf, chunk);
            if (err != MP_OK) return err;
            
            if (matches && memcmp(buf, cmp_ptr, chunk) != 0) {
              matches = false;
            }
            cmp_ptr += chunk;
            remaining -= chunk;
          }
          if (matches) {
            return MP_OK; // Match found! Decoder is positioned at the value.
          }
        }
      } else {
        // Length mismatch, just skip the string body
        err = decoder->stream->skip(decoder->stream, key_len);
        if (err != MP_OK)
          return err;
      }
    } else {
      // It's not a string key, we must skip it using the tag we already
      // consumed
      err = mp_skip_tag(decoder, tag);
      if (err != MP_OK)
        return err;
    }

    // We didn't match the key, so we must skip the value to get to the next
    // pair
    err = mp_skip(decoder);
    if (err != MP_OK)
      return err;
  }

  return MP_ERROR_QUERY_NOT_FOUND;
}

mp_error_t mp_query_array_index(mp_decoder_t *decoder, uint32_t target_index) {
  if (!decoder)
    return MP_ERROR_BAD_ARG;

  uint32_t array_len;
  mp_error_t err = mp_decode_stream_array_len(decoder->stream, &array_len);
  if (err == MP_ERROR_DECODE_INVALID_FORMAT) {
      return MP_ERROR_QUERY_TYPE_MISMATCH;
  } else if (err != MP_OK) {
      return err;
  }

  if (target_index >= array_len) {
    // Index out of bounds, consume the whole array to leave stream cleanly
    // positioned after it
    for (uint32_t i = 0; i < array_len; i++) {
      err = mp_skip(decoder);
      if (err != MP_OK)
        return err;
    }
    return MP_ERROR_QUERY_NOT_FOUND;
  }

  // Skip elements until we reach the target index
  for (uint32_t i = 0; i < target_index; i++) {
    err = mp_skip(decoder);
    if (err != MP_OK)
      return err;
  }

  return MP_OK; // Decoder is perfectly positioned at the requested index
}

// -----------------------------------------------------------------------------
// Query Path API (Concept 1: Deep Plucking)
// -----------------------------------------------------------------------------

void mp_query_init(mp_query_t *q) {
  if (q)
    q->count = 0;
}

mp_error_t mp_query_add_path_str(mp_query_t *q, const char *key) {
  if (!q || !key)
    return MP_ERROR_BAD_ARG;
  if (q->count >= MP_MAX_QUERY_DEPTH)
    return MP_ERROR_QUERY_MAX_DEPTH; // Max depth reached
  q->steps[q->count].type = MP_QUERY_PATH_STR;
  q->steps[q->count].via.str = key;
  q->count++;
  return MP_OK;
}

mp_error_t mp_query_add_path_idx(mp_query_t *q, uint32_t index) {
  if (!q)
    return MP_ERROR_BAD_ARG;
  if (q->count >= MP_MAX_QUERY_DEPTH)
    return MP_ERROR_QUERY_MAX_DEPTH; // Max depth reached
  q->steps[q->count].type = MP_QUERY_PATH_INDEX;
  q->steps[q->count].via.index = index;
  q->count++;
  return MP_OK;
}

mp_error_t mp_query_execute(mp_decoder_t *decoder, const mp_query_t *q) {
  if (!decoder || !q)
    return MP_ERROR_BAD_ARG;

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

mp_error_t mp_query_extract_fields(mp_decoder_t *decoder, mp_zone_t *zone,
                                   const char **fields, uint32_t field_count,
                                   mp_object_t *out_map) {
  if (!decoder || !zone || !fields || !out_map)
    return MP_ERROR_BAD_ARG;

  uint32_t map_len;
  mp_error_t err = mp_decode_stream_map_len(decoder->stream, &map_len);
  if (err != MP_OK)
    return err; // If it's not a map, this will correctly fail

  // Attach zone temporarily if decoder doesn't have one, or just use the passed
  // zone directly
  mp_zone_t *old_zone = decoder->zone;
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
    if (err != MP_OK)
      goto cleanup;

    uint32_t key_len = 0;
    bool is_string = false;

    // Check if the key is a string and determine its length
    switch (tag) {
    case MP_TAG_FIXSTR_MIN ... MP_TAG_FIXSTR_MAX: // warning: compiler extension
      key_len = tag & 0x1f;
      is_string = true;
      break;
    case MP_TAG_STR8: {
      uint8_t len8;
      err = decoder->stream->read(decoder->stream, &len8, 1);
      if (err != MP_OK)
        goto cleanup;
      key_len = len8;
      is_string = true;
      break;
    }
    case MP_TAG_STR16: {
      char len16[2];
      err = decoder->stream->read(decoder->stream, len16, 2);
      if (err != MP_OK)
        goto cleanup;
      key_len = ((uint32_t)(uint8_t)len16[0] << 8) | (uint8_t)len16[1];
      is_string = true;
      break;
    }
    case MP_TAG_STR32: {
      char len32[4];
      err = decoder->stream->read(decoder->stream, len32, 4);
      if (err != MP_OK)
        goto cleanup;
      key_len = ((uint32_t)(uint8_t)len32[0] << 24) |
                ((uint32_t)(uint8_t)len32[1] << 16) |
                ((uint32_t)(uint8_t)len32[2] << 8) | (uint8_t)len32[3];
      is_string = true;
      break;
    }
    }

    bool matched = false;

    if (is_string) {
      // Read the key string into a stack buffer for comparison
      char buf[256];
      char *key_ptr = buf;
      bool alloc_key = false;

      if (key_len >= sizeof(buf)) {
        // If it's huge, we dynamically allocate it to the zone right away
        key_ptr = (char *)mp_zone_alloc(zone, key_len);
        if (!key_ptr) {
          err = MP_ERROR_NOMEM;
          goto cleanup;
        }
        alloc_key = true;
      }

      err = decoder->stream->read(decoder->stream, key_ptr, key_len);
      if (err != MP_OK)
        goto cleanup;

      // Check against requested fields
      for (uint32_t j = 0; j < field_count; j++) {
        if (strlen(fields[j]) == key_len &&
            memcmp(fields[j], key_ptr, key_len) == 0) {
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
          char *final_key_ptr = (char *)mp_zone_alloc(zone, key_len + 1);
          if (!final_key_ptr) {
            err = MP_ERROR_NOMEM;
            goto cleanup;
          }
          memcpy(final_key_ptr, key_ptr, key_len);
          final_key_ptr[key_len] =
              '\0'; // Optional null-terminator for convenience
          key_ptr = final_key_ptr;
        }

        // Attach key to AST
        out_map->via.map.ptr[extracted].key.type = MP_TYPE_STR;
        out_map->via.map.ptr[extracted].key.via.str.ptr = key_ptr;
        out_map->via.map.ptr[extracted].key.via.str.size = key_len;

        // Decode value into AST
        err = mp_decode(decoder, &out_map->via.map.ptr[extracted].val);
        if (err != MP_OK)
          goto cleanup;

        extracted++;
      }
    } else {
      // Not a string key, skip it
      err = mp_skip_tag(decoder, tag);
      if (err != MP_OK)
        goto cleanup;
    }

    if (!matched) {
      // We read or skipped the key, but it didn't match. Skip the value.
      err = mp_skip(decoder);
      if (err != MP_OK)
        goto cleanup;
    }
  }

  // Shrink the map size to exactly how many fields we successfully extracted
  out_map->via.map.size = extracted;

cleanup:
  decoder->zone = old_zone;
  return err;
}

// -----------------------------------------------------------------------------
// Array Filtering API (Concept 3: Searching by Value)
// -----------------------------------------------------------------------------

static inline bool mp_is_int_tag_local(uint8_t b) {
  if (b <= MP_TAG_FIXINT_MAX)
    return true;
  if (b >= MP_TAG_NEGFIXINT_MIN)
    return true;
  if (b >= MP_TAG_INT8 && b <= MP_TAG_INT64)
    return true;
  if (b >= MP_TAG_UINT8 && b <= MP_TAG_UINT64)
    return true;
  return false;
}

static inline bool mp_is_str_tag_local(uint8_t b) {
  if (b >= MP_TAG_FIXSTR_MIN && b <= MP_TAG_FIXSTR_MAX)
    return true;
  if (b >= MP_TAG_STR8 && b <= MP_TAG_STR32)
    return true;
  return false;
}

static mp_error_t mp_decode_stream_int_from_tag_local(mp_stream_t *stream,
                                                      uint8_t b, int64_t *out) {
  if (b <= MP_TAG_FIXINT_MAX) {
    *out = b;
    return MP_OK;
  }
  if (b >= MP_TAG_NEGFIXINT_MIN) {
    *out = (int8_t)b;
    return MP_OK;
  }

  char buf[8];
  switch (b) {
  case MP_TAG_INT8:
    if (stream->read(stream, buf, 1) != MP_OK)
      return MP_ERROR_DECODE_INCOMPLETE;
    *out = (int8_t)buf[0];
    return MP_OK;
  case MP_TAG_INT16:
    if (stream->read(stream, buf, 2) != MP_OK)
      return MP_ERROR_DECODE_INCOMPLETE;
    *out = (int16_t)(((uint16_t)(uint8_t)buf[0] << 8) | (uint8_t)buf[1]);
    return MP_OK;
  case MP_TAG_INT32:
    if (stream->read(stream, buf, 4) != MP_OK)
      return MP_ERROR_DECODE_INCOMPLETE;
    *out = (int32_t)(((uint32_t)(uint8_t)buf[0] << 24) |
                     ((uint32_t)(uint8_t)buf[1] << 16) |
                     ((uint32_t)(uint8_t)buf[2] << 8) | (uint8_t)buf[3]);
    return MP_OK;
  case MP_TAG_INT64:
    if (stream->read(stream, buf, 8) != MP_OK)
      return MP_ERROR_DECODE_INCOMPLETE;
    *out = (int64_t)(((uint64_t)(uint8_t)buf[0] << 56) |
                     ((uint64_t)(uint8_t)buf[1] << 48) |
                     ((uint64_t)(uint8_t)buf[2] << 40) |
                     ((uint64_t)(uint8_t)buf[3] << 32) |
                     ((uint64_t)(uint8_t)buf[4] << 24) |
                     ((uint64_t)(uint8_t)buf[5] << 16) |
                     ((uint64_t)(uint8_t)buf[6] << 8) | (uint8_t)buf[7]);
    return MP_OK;
  case MP_TAG_UINT8:
    if (stream->read(stream, buf, 1) != MP_OK)
      return MP_ERROR_DECODE_INCOMPLETE;
    *out = (uint8_t)buf[0];
    return MP_OK;
  case MP_TAG_UINT16:
    if (stream->read(stream, buf, 2) != MP_OK)
      return MP_ERROR_DECODE_INCOMPLETE;
    *out = (((uint16_t)(uint8_t)buf[0] << 8) | (uint8_t)buf[1]);
    return MP_OK;
  case MP_TAG_UINT32:
    if (stream->read(stream, buf, 4) != MP_OK)
      return MP_ERROR_DECODE_INCOMPLETE;
    *out =
        (((uint32_t)(uint8_t)buf[0] << 24) | ((uint32_t)(uint8_t)buf[1] << 16) |
         ((uint32_t)(uint8_t)buf[2] << 8) | (uint8_t)buf[3]);
    return MP_OK;
  case MP_TAG_UINT64:
    if (stream->read(stream, buf, 8) != MP_OK)
      return MP_ERROR_DECODE_INCOMPLETE;
    *out = (int64_t)(((uint64_t)(uint8_t)buf[0] << 56) |
                     ((uint64_t)(uint8_t)buf[1] << 48) |
                     ((uint64_t)(uint8_t)buf[2] << 40) |
                     ((uint64_t)(uint8_t)buf[3] << 32) |
                     ((uint64_t)(uint8_t)buf[4] << 24) |
                     ((uint64_t)(uint8_t)buf[5] << 16) |
                     ((uint64_t)(uint8_t)buf[6] << 8) | (uint8_t)buf[7]);
    return MP_OK;
  }
  return MP_ERROR_DECODE_INVALID_FORMAT;
}

static mp_error_t mp_decode_stream_str_len_from_tag_local(mp_stream_t *stream,
                                                          uint8_t b,
                                                          uint32_t *out_len) {
  if (b >= MP_TAG_FIXSTR_MIN && b <= MP_TAG_FIXSTR_MAX) {
    *out_len = b & 0x1f;
    return MP_OK;
  }
  char buf[4];
  switch (b) {
  case MP_TAG_STR8:
    if (stream->read(stream, buf, 1) != MP_OK)
      return MP_ERROR_DECODE_INCOMPLETE;
    *out_len = (uint8_t)buf[0];
    return MP_OK;
  case MP_TAG_STR16:
    if (stream->read(stream, buf, 2) != MP_OK)
      return MP_ERROR_DECODE_INCOMPLETE;
    *out_len = (((uint16_t)(uint8_t)buf[0] << 8) | (uint8_t)buf[1]);
    return MP_OK;
  case MP_TAG_STR32:
    if (stream->read(stream, buf, 4) != MP_OK)
      return MP_ERROR_DECODE_INCOMPLETE;
    *out_len =
        (((uint32_t)(uint8_t)buf[0] << 24) | ((uint32_t)(uint8_t)buf[1] << 16) |
         ((uint32_t)(uint8_t)buf[2] << 8) | (uint8_t)buf[3]);
    return MP_OK;
  }
  return MP_ERROR_DECODE_INVALID_FORMAT;
}

void mp_filter_init(mp_filter_t *f) {
  if (f)
    f->count = 0;
}

mp_error_t mp_filter_add_str(mp_filter_t *f, const char *key, const char *val) {
  if (!f || !key || !val)
    return MP_ERROR_BAD_ARG;
  if (f->count >= MP_MAX_FILTER_CONDITIONS)
    return MP_ERROR_BAD_ARG;
  f->conditions[f->count].key = key;
  f->conditions[f->count].type = MP_FILTER_TYPE_STR;
  f->conditions[f->count].expected.s = val;
  f->count++;
  return MP_OK;
}

mp_error_t mp_filter_add_int(mp_filter_t *f, const char *key, int64_t val) {
  if (!f || !key)
    return MP_ERROR_BAD_ARG;
  if (f->count >= MP_MAX_FILTER_CONDITIONS)
    return MP_ERROR_BAD_ARG;
  f->conditions[f->count].key = key;
  f->conditions[f->count].type = MP_FILTER_TYPE_INT;
  f->conditions[f->count].expected.i = val;
  f->count++;
  return MP_OK;
}

mp_error_t mp_filter_add_bool(mp_filter_t *f, const char *key, bool val) {
  if (!f || !key)
    return MP_ERROR_BAD_ARG;
  if (f->count >= MP_MAX_FILTER_CONDITIONS)
    return MP_ERROR_BAD_ARG;
  f->conditions[f->count].key = key;
  f->conditions[f->count].type = MP_FILTER_TYPE_BOOL;
  f->conditions[f->count].expected.b = val;
  f->count++;
  return MP_OK;
}

static inline mp_error_t match_filter_key(mp_decoder_t *decoder,
                                          const mp_filter_t *filter,
                                          int *out_match_idx) {
  *out_match_idx = -1;

  uint8_t tag;
  mp_error_t err = decoder->stream->read(decoder->stream, &tag, 1);
  if (err != MP_OK)
    return err;

  uint32_t key_len = 0;
  bool is_string = false;

  switch (tag) {
  case MP_TAG_FIXSTR_MIN ... MP_TAG_FIXSTR_MAX: // warning: compiler extension
    key_len = tag & 0x1f;
    is_string = true;
    break;
  case MP_TAG_STR8: {
    uint8_t len8;
    err = decoder->stream->read(decoder->stream, &len8, 1);
    if (err != MP_OK)
      return err;
    key_len = len8;
    is_string = true;
    break;
  }
  case MP_TAG_STR16: {
    char len16[2];
    err = decoder->stream->read(decoder->stream, len16, 2);
    if (err != MP_OK)
      return err;
    key_len = ((uint32_t)(uint8_t)len16[0] << 8) | (uint8_t)len16[1];
    is_string = true;
    break;
  }
  case MP_TAG_STR32: {
    char len32[4];
    err = decoder->stream->read(decoder->stream, len32, 4);
    if (err != MP_OK)
      return err;
    key_len = ((uint32_t)(uint8_t)len32[0] << 24) |
              ((uint32_t)(uint8_t)len32[1] << 16) |
              ((uint32_t)(uint8_t)len32[2] << 8) | (uint8_t)len32[3];
    is_string = true;
    break;
  }
  }

  if (is_string) {
    char buf[256];
    if (key_len <= sizeof(buf)) {
      err = decoder->stream->read(decoder->stream, buf, key_len);
      if (err != MP_OK)
        return err;

      for (uint32_t f_idx = 0; f_idx < filter->count; f_idx++) {
        if (strlen(filter->conditions[f_idx].key) == key_len &&
            memcmp(filter->conditions[f_idx].key, buf, key_len) == 0) {
          *out_match_idx = f_idx;
          break;
        }
      }
    } else {
      // Stream key is large. To avoid allocations, compare chunks against all valid filter conditions
      uint32_t remaining = key_len;
      uint32_t offset = 0;
      
      // Track which filters are still potential matches
      bool possible_matches[MP_MAX_FILTER_CONDITIONS];
      for (uint32_t f_idx = 0; f_idx < filter->count; f_idx++) {
          possible_matches[f_idx] = (strlen(filter->conditions[f_idx].key) == key_len);
      }

      while (remaining > 0) {
        uint32_t chunk = remaining < sizeof(buf) ? remaining : sizeof(buf);
        err = decoder->stream->read(decoder->stream, buf, chunk);
        if (err != MP_OK) return err;

        for (uint32_t f_idx = 0; f_idx < filter->count; f_idx++) {
            if (possible_matches[f_idx]) {
                if (memcmp(buf, filter->conditions[f_idx].key + offset, chunk) != 0) {
                    possible_matches[f_idx] = false;
                }
            }
        }
        offset += chunk;
        remaining -= chunk;
      }

      for (uint32_t f_idx = 0; f_idx < filter->count; f_idx++) {
          if (possible_matches[f_idx]) {
              *out_match_idx = f_idx;
              break;
          }
      }
    }
  } else {
    err = mp_skip_tag(decoder, tag);
    if (err != MP_OK)
      return err;
  }

  return MP_OK;
}

static inline mp_error_t match_filter_value(mp_decoder_t *decoder,
                                            const mp_filter_condition_t *cond,
                                            bool *out_matched) {
  *out_matched = false;

  uint8_t val_tag;
  mp_error_t err = decoder->stream->read(decoder->stream, &val_tag, 1);
  if (err != MP_OK)
    return err;

  switch (cond->type) {
  case MP_FILTER_TYPE_INT:
    if (mp_is_int_tag_local(val_tag)) {
      int64_t actual_val;
      err = mp_decode_stream_int_from_tag_local(decoder->stream, val_tag,
                                                &actual_val);
      if (err != MP_OK)
        return err;
      if (actual_val == cond->expected.i)
        *out_matched = true;
    } else {
      err = mp_skip_tag(decoder, val_tag);
      if (err != MP_OK)
        return err;
    }
    break;
  case MP_FILTER_TYPE_BOOL:
    if (val_tag == MP_TAG_TRUE && cond->expected.b == true) {
      *out_matched = true;
    } else if (val_tag == MP_TAG_FALSE && cond->expected.b == false) {
      *out_matched = true;
    } else {
      err = mp_skip_tag(decoder, val_tag);
      if (err != MP_OK)
        return err;
    }
    break;
  case MP_FILTER_TYPE_STR:
    if (mp_is_str_tag_local(val_tag)) {
      uint32_t val_str_len;
      err = mp_decode_stream_str_len_from_tag_local(decoder->stream, val_tag,
                                                    &val_str_len);
      if (err != MP_OK)
        return err;

      size_t expected_len = strlen(cond->expected.s);
      if (val_str_len == expected_len) {
        char val_buf[256];
        if (val_str_len <= sizeof(val_buf)) {
          err = decoder->stream->read(decoder->stream, val_buf, val_str_len);
          if (err != MP_OK)
            return err;
          if (memcmp(val_buf, cond->expected.s, val_str_len) == 0) {
            *out_matched = true;
          }
        } else {
          uint32_t remaining = val_str_len;
          const char *cmp_ptr = cond->expected.s;
          bool matches = true;
          while (remaining > 0) {
            uint32_t chunk = remaining < sizeof(val_buf) ? remaining : sizeof(val_buf);
            err = decoder->stream->read(decoder->stream, val_buf, chunk);
            if (err != MP_OK) return err;
            if (matches && memcmp(val_buf, cmp_ptr, chunk) != 0) {
              matches = false;
            }
            cmp_ptr += chunk;
            remaining -= chunk;
          }
          if (matches) {
            *out_matched = true;
          }
        }
      } else {
        err = decoder->stream->skip(decoder->stream, val_str_len);
        if (err != MP_OK)
          return err;
      }
    } else {
      err = mp_skip_tag(decoder, val_tag);
      if (err != MP_OK)
        return err;
    }
    break;
  }

  return MP_OK;
}

mp_error_t mp_query_filter_array(mp_decoder_t *decoder, mp_zone_t *zone,
                                 const mp_filter_t *filter,
                                 mp_object_t *out_array) {
  if (!decoder || !zone || !filter || !out_array)
    return MP_ERROR_BAD_ARG;

  uint32_t array_len;
  mp_error_t err = mp_decode_stream_array_len(decoder->stream, &array_len);
  if (err != MP_OK)
    return err;

  // Attach zone temporarily if decoder doesn't have one
  mp_zone_t *old_zone = decoder->zone;
  decoder->zone = zone;

  // Pre-allocate maximum possible capacity.
  err = mp_build_array(zone, out_array, array_len);
  if (err != MP_OK)
    goto cleanup;

  uint32_t matched_count = 0;

  for (uint32_t i = 0; i < array_len; i++) {
    // Mark the start of the current map in the stream!
    err = mp_stream_mark(decoder->stream);
    if (err != MP_OK)
      goto cleanup;

    uint8_t tag;
    err = decoder->stream->read(decoder->stream, &tag, 1);
    if (err != MP_OK) goto cleanup;

    uint32_t map_len = 0;
    bool is_map = false;
    if (tag >= MP_TAG_FIXMAP_MIN && tag <= MP_TAG_FIXMAP_MAX) {
      map_len = tag & 0x0f;
      is_map = true;
    } else if (tag == MP_TAG_MAP16) {
      char len16[2];
      err = decoder->stream->read(decoder->stream, len16, 2);
      if (err != MP_OK) goto cleanup;
      map_len = ((uint32_t)(uint8_t)len16[0] << 8) | (uint8_t)len16[1];
      is_map = true;
    } else if (tag == MP_TAG_MAP32) {
      char len32[4];
      err = decoder->stream->read(decoder->stream, len32, 4);
      if (err != MP_OK) goto cleanup;
      map_len = ((uint32_t)(uint8_t)len32[0] << 24) |
                ((uint32_t)(uint8_t)len32[1] << 16) |
                ((uint32_t)(uint8_t)len32[2] << 8) | (uint8_t)len32[3];
      is_map = true;
    }

    if (!is_map) {
      err = mp_skip_tag(decoder, tag);
      if (err != MP_OK) goto cleanup;
      continue; // Skip safely and try next array element
    }

    uint32_t conditions_met = 0;

    for (uint32_t j = 0; j < map_len; j++) {
      int match_idx = -1;
      err = match_filter_key(decoder, filter, &match_idx);
      if (err != MP_OK)
        goto cleanup;

      if (match_idx != -1) {
        bool val_matched = false;
        err = match_filter_value(decoder, &filter->conditions[match_idx],
                                 &val_matched);
        if (err != MP_OK)
          goto cleanup;
        if (val_matched)
          conditions_met++;
      } else {
        // Key does not match any condition, skip the value instantly
        err = mp_skip(decoder);
        if (err != MP_OK)
          goto cleanup;
      }
    } // end of map loop

    if (conditions_met == filter->count) {
      // MATCH FOUND!
      // 1. Rewind the stream to the start of the map
      err = mp_stream_reset(decoder->stream);
      if (err != MP_OK)
        goto cleanup;

      // 2. Decode the entire map perfectly into the AST array
      err = mp_decode(decoder, &out_array->via.array.ptr[matched_count]);
      if (err != MP_OK)
        goto cleanup;

      matched_count++;
    }
  } // end of array loop

  // Shrink array
  out_array->via.array.size = matched_count;

cleanup:
  decoder->zone = old_zone;
  return err;
}
