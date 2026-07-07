#include "decoder.h"
#include <string.h>

void mp_decoder_init(mp_decoder_t *decoder, mp_stream_t *stream,
                     mp_zone_t *zone) {
  if (decoder) {
    decoder->stream = stream;
    decoder->zone = zone;
    decoder->depth = 0;
    decoder->max_depth = 256;
  }
}

static inline uint16_t deserialize_be16(const char *buf) {
  return ((uint16_t)(uint8_t)buf[0] << 8) | (uint16_t)(uint8_t)buf[1];
}

static inline uint32_t deserialize_be32(const char *buf) {
  return ((uint32_t)(uint8_t)buf[0] << 24) | ((uint32_t)(uint8_t)buf[1] << 16) |
         ((uint32_t)(uint8_t)buf[2] << 8) | (uint32_t)(uint8_t)buf[3];
}

static inline uint64_t deserialize_be64(const char *buf) {
  return ((uint64_t)(uint8_t)buf[0] << 56) | ((uint64_t)(uint8_t)buf[1] << 48) |
         ((uint64_t)(uint8_t)buf[2] << 40) | ((uint64_t)(uint8_t)buf[3] << 32) |
         ((uint64_t)(uint8_t)buf[4] << 24) | ((uint64_t)(uint8_t)buf[5] << 16) |
         ((uint64_t)(uint8_t)buf[6] << 8) | (uint64_t)(uint8_t)buf[7];
}

static mp_error_t read_bytes(mp_decoder_t *decoder, void *buf, size_t len) {
  if (len == 0)
    return MP_OK;
  if (!decoder->stream || !decoder->stream->read)
    return MP_ERROR_BAD_ARG;
  return decoder->stream->read(decoder->stream, buf, len);
}

static mp_error_t read_string_to_zone(mp_decoder_t *decoder,
                                      mp_object_t *out_obj, uint32_t len) {
  out_obj->type = MP_TYPE_STR;
  out_obj->via.str.size = len;
  out_obj->via.str.ptr =
      len > 0 ? (char *)mp_zone_alloc(decoder->zone, len) : NULL;
  if (!out_obj->via.str.ptr && len > 0)
    return MP_ERROR_NOMEM;
  mp_error_t err = read_bytes(decoder, (void *)out_obj->via.str.ptr, len);
  if (err != MP_OK)
    return MP_ERROR_DECODE_TRUNCATED_STR;
  return MP_OK;
}

static mp_error_t read_bin_to_zone(mp_decoder_t *decoder, mp_object_t *out_obj,
                                   uint32_t len) {
  out_obj->type = MP_TYPE_BIN;
  out_obj->via.bin.size = len;
  out_obj->via.bin.ptr =
      len > 0 ? (char *)mp_zone_alloc(decoder->zone, len) : NULL;
  if (!out_obj->via.bin.ptr && len > 0)
    return MP_ERROR_NOMEM;
  mp_error_t err = read_bytes(decoder, (void *)out_obj->via.bin.ptr, len);
  if (err != MP_OK)
    return MP_ERROR_DECODE_TRUNCATED_BIN;
  return MP_OK;
}

static mp_error_t read_ext_to_zone(mp_decoder_t *decoder, mp_object_t *out_obj,
                                   uint32_t len) {
  uint8_t ext_type;
  mp_error_t err = read_bytes(decoder, &ext_type, 1);
  if (err != MP_OK)
    return MP_ERROR_DECODE_TRUNCATED_EXT;

  out_obj->type = MP_TYPE_EXT;
  out_obj->via.ext.type = (int8_t)ext_type;
  out_obj->via.ext.size = len;
  out_obj->via.ext.ptr =
      len > 0 ? (char *)mp_zone_alloc(decoder->zone, len) : NULL;
  if (!out_obj->via.ext.ptr && len > 0)
    return MP_ERROR_NOMEM;

  err = read_bytes(decoder, (void *)out_obj->via.ext.ptr, len);
  if (err != MP_OK)
    return MP_ERROR_DECODE_TRUNCATED_EXT;
  return MP_OK;
}

static mp_error_t mp_decode_internal(mp_decoder_t *decoder,
                                     mp_object_t *out_obj) {
  if (decoder->depth >= decoder->max_depth)
    return MP_ERROR_DECODE_DEPTH_EXCEEDED;

  uint8_t b;
  mp_error_t err = read_bytes(decoder, &b, 1);
  if (err != MP_OK)
    return err;

  if (b <= MP_TAG_FIXINT_MAX) {
    out_obj->type = MP_TYPE_POSITIVE_INTEGER;
    out_obj->via.u64 = b;
    return MP_OK;
  } else if (b >= MP_TAG_NEGFIXINT_MIN) {
    out_obj->type = MP_TYPE_NEGATIVE_INTEGER;
    out_obj->via.i64 = (int8_t)b;
    return MP_OK;
  } else if (b >= MP_TAG_FIXSTR_MIN && b <= MP_TAG_FIXSTR_MAX) {
    return read_string_to_zone(decoder, out_obj, b & 0x1f);
  } else if (b >= MP_TAG_FIXARRAY_MIN && b <= MP_TAG_FIXARRAY_MAX) {
    uint32_t len = b & 0x0f;
    out_obj->type = MP_TYPE_ARRAY;
    out_obj->via.array.size = len;
    out_obj->via.array.ptr =
        len > 0 ? (mp_object_t *)mp_zone_alloc(decoder->zone,
                                               sizeof(mp_object_t) * len)
                : NULL;
    if (!out_obj->via.array.ptr && len > 0)
      return MP_ERROR_NOMEM;
    for (uint32_t i = 0; i < len; ++i) {
      decoder->depth++;
      err = mp_decode_internal(decoder, &out_obj->via.array.ptr[i]);
      decoder->depth--;
      if (err != MP_OK)
        return err;
    }
    return MP_OK;
  } else if (b >= MP_TAG_FIXMAP_MIN && b <= MP_TAG_FIXMAP_MAX) {
    uint32_t len = b & 0x0f;
    out_obj->type = MP_TYPE_MAP;
    out_obj->via.map.size = len;
    out_obj->via.map.ptr =
        len > 0 ? (mp_object_kv_t *)mp_zone_alloc(decoder->zone,
                                                  sizeof(mp_object_kv_t) * len)
                : NULL;
    if (!out_obj->via.map.ptr && len > 0)
      return MP_ERROR_NOMEM;
    for (uint32_t i = 0; i < len; ++i) {
      out_obj->via.map.ptr[i].key =
          (mp_object_t *)mp_zone_alloc(decoder->zone, sizeof(mp_object_t));
      out_obj->via.map.ptr[i].val =
          (mp_object_t *)mp_zone_alloc(decoder->zone, sizeof(mp_object_t));
      if (!out_obj->via.map.ptr[i].key || !out_obj->via.map.ptr[i].val)
        return MP_ERROR_NOMEM;

      decoder->depth++;
      err = mp_decode_internal(decoder, out_obj->via.map.ptr[i].key);
      if (err == MP_OK)
        err = mp_decode_internal(decoder, out_obj->via.map.ptr[i].val);
      decoder->depth--;
      if (err != MP_OK)
        return err;
    }
    return MP_OK;
  }

  switch (b) {
  case MP_TAG_NIL:
    out_obj->type = MP_TYPE_NIL;
    break;
  case MP_TAG_FALSE:
    out_obj->type = MP_TYPE_BOOLEAN;
    out_obj->via.boolean = false;
    break;
  case MP_TAG_TRUE:
    out_obj->type = MP_TYPE_BOOLEAN;
    out_obj->via.boolean = true;
    break;
  case MP_TAG_FLOAT32: {
    char buf[4];
    if (read_bytes(decoder, buf, 4) != MP_OK)
      return MP_ERROR_DECODE_INCOMPLETE;
    union {
      float f;
      uint32_t u;
    } u;
    u.u = deserialize_be32(buf);
    out_obj->type = MP_TYPE_FLOAT32;
    out_obj->via.f32 = u.f;
    break;
  }
  case MP_TAG_FLOAT64: {
    char buf[8];
    if (read_bytes(decoder, buf, 8) != MP_OK)
      return MP_ERROR_DECODE_INCOMPLETE;
    union {
      double d;
      uint64_t u;
    } u;
    u.u = deserialize_be64(buf);
    out_obj->type = MP_TYPE_FLOAT64;
    out_obj->via.f64 = u.d;
    break;
  }
  case MP_TAG_UINT8: {
    char buf[1];
    if (read_bytes(decoder, buf, 1) != MP_OK)
      return MP_ERROR_DECODE_INCOMPLETE;
    out_obj->type = MP_TYPE_POSITIVE_INTEGER;
    out_obj->via.u64 = (uint8_t)buf[0];
    break;
  }
  case MP_TAG_UINT16: {
    char buf[2];
    if (read_bytes(decoder, buf, 2) != MP_OK)
      return MP_ERROR_DECODE_INCOMPLETE;
    out_obj->type = MP_TYPE_POSITIVE_INTEGER;
    out_obj->via.u64 = deserialize_be16(buf);
    break;
  }
  case MP_TAG_UINT32: {
    char buf[4];
    if (read_bytes(decoder, buf, 4) != MP_OK)
      return MP_ERROR_DECODE_INCOMPLETE;
    out_obj->type = MP_TYPE_POSITIVE_INTEGER;
    out_obj->via.u64 = deserialize_be32(buf);
    break;
  }
  case MP_TAG_UINT64: {
    char buf[8];
    if (read_bytes(decoder, buf, 8) != MP_OK)
      return MP_ERROR_DECODE_INCOMPLETE;
    out_obj->type = MP_TYPE_POSITIVE_INTEGER;
    out_obj->via.u64 = deserialize_be64(buf);
    break;
  }
  case MP_TAG_INT8: {
    char buf[1];
    if (read_bytes(decoder, buf, 1) != MP_OK)
      return MP_ERROR_DECODE_INCOMPLETE;
    out_obj->type = MP_TYPE_NEGATIVE_INTEGER;
    out_obj->via.i64 = (int8_t)buf[0];
    break;
  }
  case MP_TAG_INT16: {
    char buf[2];
    if (read_bytes(decoder, buf, 2) != MP_OK)
      return MP_ERROR_DECODE_INCOMPLETE;
    out_obj->type = MP_TYPE_NEGATIVE_INTEGER;
    out_obj->via.i64 = (int16_t)deserialize_be16(buf);
    break;
  }
  case MP_TAG_INT32: {
    char buf[4];
    if (read_bytes(decoder, buf, 4) != MP_OK)
      return MP_ERROR_DECODE_INCOMPLETE;
    out_obj->type = MP_TYPE_NEGATIVE_INTEGER;
    out_obj->via.i64 = (int32_t)deserialize_be32(buf);
    break;
  }
  case MP_TAG_INT64: {
    char buf[8];
    if (read_bytes(decoder, buf, 8) != MP_OK)
      return MP_ERROR_DECODE_INCOMPLETE;
    out_obj->type = MP_TYPE_NEGATIVE_INTEGER;
    out_obj->via.i64 = (int64_t)deserialize_be64(buf);
    break;
  }
  case MP_TAG_STR8: {
    char buf[1];
    if (read_bytes(decoder, buf, 1) != MP_OK)
      return MP_ERROR_DECODE_TRUNCATED_STR;
    return read_string_to_zone(decoder, out_obj, (uint8_t)buf[0]);
  }
  case MP_TAG_STR16: {
    char buf[2];
    if (read_bytes(decoder, buf, 2) != MP_OK)
      return MP_ERROR_DECODE_TRUNCATED_STR;
    return read_string_to_zone(decoder, out_obj, deserialize_be16(buf));
  }
  case MP_TAG_STR32: {
    char buf[4];
    if (read_bytes(decoder, buf, 4) != MP_OK)
      return MP_ERROR_DECODE_TRUNCATED_STR;
    return read_string_to_zone(decoder, out_obj, deserialize_be32(buf));
  }
  case MP_TAG_BIN8: {
    char buf[1];
    if (read_bytes(decoder, buf, 1) != MP_OK)
      return MP_ERROR_DECODE_TRUNCATED_BIN;
    return read_bin_to_zone(decoder, out_obj, (uint8_t)buf[0]);
  }
  case MP_TAG_BIN16: {
    char buf[2];
    if (read_bytes(decoder, buf, 2) != MP_OK)
      return MP_ERROR_DECODE_TRUNCATED_BIN;
    return read_bin_to_zone(decoder, out_obj, deserialize_be16(buf));
  }
  case MP_TAG_BIN32: {
    char buf[4];
    if (read_bytes(decoder, buf, 4) != MP_OK)
      return MP_ERROR_DECODE_TRUNCATED_BIN;
    return read_bin_to_zone(decoder, out_obj, deserialize_be32(buf));
  }
  case MP_TAG_ARRAY16: {
    char buf[2];
    if (read_bytes(decoder, buf, 2) != MP_OK)
      return MP_ERROR_DECODE_TRUNCATED_ARRAY;
    uint32_t len = deserialize_be16(buf);
    out_obj->type = MP_TYPE_ARRAY;
    out_obj->via.array.size = len;
    out_obj->via.array.ptr =
        len > 0 ? (mp_object_t *)mp_zone_alloc(decoder->zone,
                                               sizeof(mp_object_t) * len)
                : NULL;
    if (!out_obj->via.array.ptr && len > 0)
      return MP_ERROR_NOMEM;
    for (uint32_t i = 0; i < len; ++i) {
      decoder->depth++;
      err = mp_decode_internal(decoder, &out_obj->via.array.ptr[i]);
      decoder->depth--;
      if (err != MP_OK)
        return err;
    }
    break;
  }
  case MP_TAG_ARRAY32: {
    char buf[4];
    if (read_bytes(decoder, buf, 4) != MP_OK)
      return MP_ERROR_DECODE_TRUNCATED_ARRAY;
    uint32_t len = deserialize_be32(buf);
    out_obj->type = MP_TYPE_ARRAY;
    out_obj->via.array.size = len;
    out_obj->via.array.ptr =
        len > 0 ? (mp_object_t *)mp_zone_alloc(decoder->zone,
                                               sizeof(mp_object_t) * len)
                : NULL;
    if (!out_obj->via.array.ptr && len > 0)
      return MP_ERROR_NOMEM;
    for (uint32_t i = 0; i < len; ++i) {
      decoder->depth++;
      err = mp_decode_internal(decoder, &out_obj->via.array.ptr[i]);
      decoder->depth--;
      if (err != MP_OK)
        return err;
    }
    break;
  }
  case MP_TAG_MAP16: {
    char buf[2];
    if (read_bytes(decoder, buf, 2) != MP_OK)
      return MP_ERROR_DECODE_TRUNCATED_MAP;
    uint32_t len = deserialize_be16(buf);
    out_obj->type = MP_TYPE_MAP;
    out_obj->via.map.size = len;
    out_obj->via.map.ptr =
        len > 0 ? (mp_object_kv_t *)mp_zone_alloc(decoder->zone,
                                                  sizeof(mp_object_kv_t) * len)
                : NULL;
    if (!out_obj->via.map.ptr && len > 0)
      return MP_ERROR_NOMEM;
    for (uint32_t i = 0; i < len; ++i) {
      out_obj->via.map.ptr[i].key =
          (mp_object_t *)mp_zone_alloc(decoder->zone, sizeof(mp_object_t));
      out_obj->via.map.ptr[i].val =
          (mp_object_t *)mp_zone_alloc(decoder->zone, sizeof(mp_object_t));
      if (!out_obj->via.map.ptr[i].key || !out_obj->via.map.ptr[i].val)
        return MP_ERROR_NOMEM;
      decoder->depth++;
      err = mp_decode_internal(decoder, out_obj->via.map.ptr[i].key);
      if (err == MP_OK)
        err = mp_decode_internal(decoder, out_obj->via.map.ptr[i].val);
      decoder->depth--;
      if (err != MP_OK)
        return err;
    }
    break;
  }
  case MP_TAG_MAP32: {
    char buf[4];
    if (read_bytes(decoder, buf, 4) != MP_OK)
      return MP_ERROR_DECODE_TRUNCATED_MAP;
    uint32_t len = deserialize_be32(buf);
    out_obj->type = MP_TYPE_MAP;
    out_obj->via.map.size = len;
    out_obj->via.map.ptr =
        len > 0 ? (mp_object_kv_t *)mp_zone_alloc(decoder->zone,
                                                  sizeof(mp_object_kv_t) * len)
                : NULL;
    if (!out_obj->via.map.ptr && len > 0)
      return MP_ERROR_NOMEM;
    for (uint32_t i = 0; i < len; ++i) {
      out_obj->via.map.ptr[i].key =
          (mp_object_t *)mp_zone_alloc(decoder->zone, sizeof(mp_object_t));
      out_obj->via.map.ptr[i].val =
          (mp_object_t *)mp_zone_alloc(decoder->zone, sizeof(mp_object_t));
      if (!out_obj->via.map.ptr[i].key || !out_obj->via.map.ptr[i].val)
        return MP_ERROR_NOMEM;
      decoder->depth++;
      err = mp_decode_internal(decoder, out_obj->via.map.ptr[i].key);
      if (err == MP_OK)
        err = mp_decode_internal(decoder, out_obj->via.map.ptr[i].val);
      decoder->depth--;
      if (err != MP_OK)
        return err;
    }
    break;
  }
  case MP_TAG_FIXEXT1:
  case MP_TAG_FIXEXT2:
  case MP_TAG_FIXEXT4:
  case MP_TAG_FIXEXT8:
  case MP_TAG_FIXEXT16: {
    uint32_t len = 1 << (b - MP_TAG_FIXEXT1);
    if (b == MP_TAG_FIXEXT1)
      len = 1;
    else if (b == MP_TAG_FIXEXT2)
      len = 2;
    else if (b == MP_TAG_FIXEXT4)
      len = 4;
    else if (b == MP_TAG_FIXEXT8)
      len = 8;
    else if (b == MP_TAG_FIXEXT16)
      len = 16;
    return read_ext_to_zone(decoder, out_obj, len);
  }
  case MP_TAG_EXT8: {
    char buf[1];
    if (read_bytes(decoder, buf, 1) != MP_OK)
      return MP_ERROR_DECODE_TRUNCATED_EXT;
    return read_ext_to_zone(decoder, out_obj, (uint8_t)buf[0]);
  }
  case MP_TAG_EXT16: {
    char buf[2];
    if (read_bytes(decoder, buf, 2) != MP_OK)
      return MP_ERROR_DECODE_TRUNCATED_EXT;
    return read_ext_to_zone(decoder, out_obj, deserialize_be16(buf));
  }
  case MP_TAG_EXT32: {
    char buf[4];
    if (read_bytes(decoder, buf, 4) != MP_OK)
      return MP_ERROR_DECODE_TRUNCATED_EXT;
    return read_ext_to_zone(decoder, out_obj, deserialize_be32(buf));
  }
  default:
    return MP_ERROR_DECODE_INVALID_FORMAT;
  }
  return MP_OK;
}

mp_error_t mp_parse_memory(mp_zone_t *zone, const char *data, size_t size,
                           mp_object_t *out_obj) {
  if (!zone || !data || !out_obj)
    return MP_ERROR_BAD_ARG;

  mp_stream_t stream;
  mp_memory_stream_ctx_t ctx;
  mp_memory_stream_init_read(&stream, &ctx, data, size);

  mp_decoder_t decoder;
  mp_decoder_init(&decoder, &stream, zone);

  return mp_decode(&decoder, out_obj);
}

mp_error_t mp_decode(mp_decoder_t *decoder, mp_object_t *out_obj) {
  if (!decoder || !out_obj)
    return MP_ERROR_BAD_ARG;
  if (decoder->depth >= decoder->max_depth)
    return MP_ERROR_DECODE_DEPTH_EXCEEDED;
  return mp_decode_internal(decoder, out_obj);
}
// -------------------------------------------------------------------------
// Incremental Streaming Decoders (SAX API)
// -------------------------------------------------------------------------
static mp_error_t stream_read(mp_stream_t *stream, void *buf, size_t count) {
  if (count == 0)
    return MP_OK;
  if (!stream || !stream->read)
    return MP_ERROR_BAD_ARG;
  return stream->read(stream, buf, count);
}

mp_error_t mp_decode_stream_nil(mp_stream_t *stream) {
  uint8_t b;
  mp_error_t err = stream_read(stream, &b, 1);
  if (err != MP_OK)
    return err;
  if (b != MP_TAG_NIL)
    return MP_ERROR_DECODE_INVALID_FORMAT;
  return MP_OK;
}

mp_error_t mp_decode_stream_bool(mp_stream_t *stream, bool *out) {
  if (!out)
    return MP_ERROR_BAD_ARG;
  uint8_t b;
  mp_error_t err = stream_read(stream, &b, 1);
  if (err != MP_OK)
    return err;
  if (b == MP_TAG_TRUE)
    *out = true;
  else if (b == MP_TAG_FALSE)
    *out = false;
  else
    return MP_ERROR_DECODE_INVALID_FORMAT;
  return MP_OK;
}

mp_error_t mp_decode_stream_int(mp_stream_t *stream, int64_t *out) {
  if (!out)
    return MP_ERROR_BAD_ARG;
  uint8_t b;
  mp_error_t err = stream_read(stream, &b, 1);
  if (err != MP_OK)
    return err;

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
    if ((err = stream_read(stream, buf, 1)) != MP_OK)
      return err;
    *out = (int8_t)buf[0];
    return MP_OK;
  case MP_TAG_INT16:
    if ((err = stream_read(stream, buf, 2)) != MP_OK)
      return err;
    *out = (int16_t)deserialize_be16(buf);
    return MP_OK;
  case MP_TAG_INT32:
    if ((err = stream_read(stream, buf, 4)) != MP_OK)
      return err;
    *out = (int32_t)deserialize_be32(buf);
    return MP_OK;
  case MP_TAG_INT64:
    if ((err = stream_read(stream, buf, 8)) != MP_OK)
      return err;
    *out = (int64_t)deserialize_be64(buf);
    return MP_OK;

  case MP_TAG_UINT8:
    if ((err = stream_read(stream, buf, 1)) != MP_OK)
      return err;
    *out = (uint8_t)buf[0];
    return MP_OK;
  case MP_TAG_UINT16:
    if ((err = stream_read(stream, buf, 2)) != MP_OK)
      return err;
    *out = deserialize_be16(buf);
    return MP_OK;
  case MP_TAG_UINT32:
    if ((err = stream_read(stream, buf, 4)) != MP_OK)
      return err;
    *out = deserialize_be32(buf);
    return MP_OK;
  case MP_TAG_UINT64: {
    if ((err = stream_read(stream, buf, 8)) != MP_OK)
      return err;
    uint64_t u = deserialize_be64(buf);
    if (u > INT64_MAX)
      return MP_ERROR_DECODE_INVALID_FORMAT; // Exceeds signed int capacity
    *out = (int64_t)u;
    return MP_OK;
  }
  default:
    return MP_ERROR_DECODE_INVALID_FORMAT;
  }
}

mp_error_t mp_decode_stream_uint(mp_stream_t *stream, uint64_t *out) {
  if (!out)
    return MP_ERROR_BAD_ARG;
  uint8_t b;
  mp_error_t err = stream_read(stream, &b, 1);
  if (err != MP_OK)
    return err;

  if (b <= MP_TAG_FIXINT_MAX) {
    *out = b;
    return MP_OK;
  }
  if (b >= MP_TAG_NEGFIXINT_MIN) {
    return MP_ERROR_DECODE_INVALID_FORMAT;
  } // Negative

  char buf[8];
  switch (b) {
  case MP_TAG_UINT8:
    if ((err = stream_read(stream, buf, 1)) != MP_OK)
      return err;
    *out = (uint8_t)buf[0];
    return MP_OK;
  case MP_TAG_UINT16:
    if ((err = stream_read(stream, buf, 2)) != MP_OK)
      return err;
    *out = deserialize_be16(buf);
    return MP_OK;
  case MP_TAG_UINT32:
    if ((err = stream_read(stream, buf, 4)) != MP_OK)
      return err;
    *out = deserialize_be32(buf);
    return MP_OK;
  case MP_TAG_UINT64:
    if ((err = stream_read(stream, buf, 8)) != MP_OK)
      return err;
    *out = deserialize_be64(buf);
    return MP_OK;

  case MP_TAG_INT8: {
    if ((err = stream_read(stream, buf, 1)) != MP_OK)
      return err;
    int8_t v = (int8_t)buf[0];
    if (v < 0)
      return MP_ERROR_DECODE_INVALID_FORMAT;
    *out = v;
    return MP_OK;
  }
  case MP_TAG_INT16: {
    if ((err = stream_read(stream, buf, 2)) != MP_OK)
      return err;
    int16_t v = (int16_t)deserialize_be16(buf);
    if (v < 0)
      return MP_ERROR_DECODE_INVALID_FORMAT;
    *out = v;
    return MP_OK;
  }
  case MP_TAG_INT32: {
    if ((err = stream_read(stream, buf, 4)) != MP_OK)
      return err;
    int32_t v = (int32_t)deserialize_be32(buf);
    if (v < 0)
      return MP_ERROR_DECODE_INVALID_FORMAT;
    *out = v;
    return MP_OK;
  }
  case MP_TAG_INT64: {
    if ((err = stream_read(stream, buf, 8)) != MP_OK)
      return err;
    int64_t v = (int64_t)deserialize_be64(buf);
    if (v < 0)
      return MP_ERROR_DECODE_INVALID_FORMAT;
    *out = v;
    return MP_OK;
  }
  default:
    return MP_ERROR_DECODE_INVALID_FORMAT;
  }
}

mp_error_t mp_decode_stream_float(mp_stream_t *stream, float *out) {
  if (!out)
    return MP_ERROR_BAD_ARG;
  uint8_t b;
  mp_error_t err = stream_read(stream, &b, 1);
  if (err != MP_OK)
    return err;

  if (b == MP_TAG_FLOAT32) {
    char buf[4];
    if ((err = stream_read(stream, buf, 4)) != MP_OK)
      return err;
    union {
      float f;
      uint32_t u;
    } u;
    u.u = deserialize_be32(buf);
    *out = u.f;
    return MP_OK;
  } else if (b == MP_TAG_FLOAT64) {
    char buf[8];
    if ((err = stream_read(stream, buf, 8)) != MP_OK)
      return err;
    union {
      double d;
      uint64_t u;
    } u;
    u.u = deserialize_be64(buf);
    *out = (float)u.d;
    return MP_OK;
  }
  return MP_ERROR_DECODE_INVALID_FORMAT;
}

mp_error_t mp_decode_stream_double(mp_stream_t *stream, double *out) {
  if (!out)
    return MP_ERROR_BAD_ARG;
  uint8_t b;
  mp_error_t err = stream_read(stream, &b, 1);
  if (err != MP_OK)
    return err;

  if (b == MP_TAG_FLOAT64) {
    char buf[8];
    if ((err = stream_read(stream, buf, 8)) != MP_OK)
      return err;
    union {
      double d;
      uint64_t u;
    } u;
    u.u = deserialize_be64(buf);
    *out = u.d;
    return MP_OK;
  } else if (b == MP_TAG_FLOAT32) {
    char buf[4];
    if ((err = stream_read(stream, buf, 4)) != MP_OK)
      return err;
    union {
      float f;
      uint32_t u;
    } u;
    u.u = deserialize_be32(buf);
    *out = (double)u.f;
    return MP_OK;
  }
  return MP_ERROR_DECODE_INVALID_FORMAT;
}

mp_error_t mp_decode_stream_str_len(mp_stream_t *stream, uint32_t *out_len) {
  if (!out_len)
    return MP_ERROR_BAD_ARG;
  uint8_t b;
  mp_error_t err = stream_read(stream, &b, 1);
  if (err != MP_OK)
    return err;

  if (b >= MP_TAG_FIXSTR_MIN && b <= MP_TAG_FIXSTR_MAX) {
    *out_len = b & 0x1f;
    return MP_OK;
  }

  char buf[4];
  switch (b) {
  case MP_TAG_STR8:
    if ((err = stream_read(stream, buf, 1)) != MP_OK)
      return err;
    *out_len = (uint8_t)buf[0];
    return MP_OK;
  case MP_TAG_STR16:
    if ((err = stream_read(stream, buf, 2)) != MP_OK)
      return err;
    *out_len = deserialize_be16(buf);
    return MP_OK;
  case MP_TAG_STR32:
    if ((err = stream_read(stream, buf, 4)) != MP_OK)
      return err;
    *out_len = deserialize_be32(buf);
    return MP_OK;
  }
  return MP_ERROR_DECODE_INVALID_FORMAT;
}

mp_error_t mp_decode_stream_bin_len(mp_stream_t *stream, uint32_t *out_len) {
  if (!out_len)
    return MP_ERROR_BAD_ARG;
  uint8_t b;
  mp_error_t err = stream_read(stream, &b, 1);
  if (err != MP_OK)
    return err;

  char buf[4];
  switch (b) {
  case MP_TAG_BIN8:
    if ((err = stream_read(stream, buf, 1)) != MP_OK)
      return err;
    *out_len = (uint8_t)buf[0];
    return MP_OK;
  case MP_TAG_BIN16:
    if ((err = stream_read(stream, buf, 2)) != MP_OK)
      return err;
    *out_len = deserialize_be16(buf);
    return MP_OK;
  case MP_TAG_BIN32:
    if ((err = stream_read(stream, buf, 4)) != MP_OK)
      return err;
    *out_len = deserialize_be32(buf);
    return MP_OK;
  }
  return MP_ERROR_DECODE_INVALID_FORMAT;
}

mp_error_t mp_decode_stream_array_len(mp_stream_t *stream, uint32_t *out_len) {
  if (!out_len)
    return MP_ERROR_BAD_ARG;
  uint8_t b;
  mp_error_t err = stream_read(stream, &b, 1);
  if (err != MP_OK)
    return err;

  if (b >= MP_TAG_FIXARRAY_MIN && b <= MP_TAG_FIXARRAY_MAX) {
    *out_len = b & 0x0f;
    return MP_OK;
  }

  char buf[4];
  switch (b) {
  case MP_TAG_ARRAY16:
    if ((err = stream_read(stream, buf, 2)) != MP_OK)
      return err;
    *out_len = deserialize_be16(buf);
    return MP_OK;
  case MP_TAG_ARRAY32:
    if ((err = stream_read(stream, buf, 4)) != MP_OK)
      return err;
    *out_len = deserialize_be32(buf);
    return MP_OK;
  }
  return MP_ERROR_DECODE_INVALID_FORMAT;
}

mp_error_t mp_decode_stream_map_len(mp_stream_t *stream, uint32_t *out_len) {
  if (!out_len)
    return MP_ERROR_BAD_ARG;
  uint8_t b;
  mp_error_t err = stream_read(stream, &b, 1);
  if (err != MP_OK)
    return err;

  if (b >= MP_TAG_FIXMAP_MIN && b <= MP_TAG_FIXMAP_MAX) {
    *out_len = b & 0x0f;
    return MP_OK;
  }

  char buf[4];
  switch (b) {
  case MP_TAG_MAP16:
    if ((err = stream_read(stream, buf, 2)) != MP_OK)
      return err;
    *out_len = deserialize_be16(buf);
    return MP_OK;
  case MP_TAG_MAP32:
    if ((err = stream_read(stream, buf, 4)) != MP_OK)
      return err;
    *out_len = deserialize_be32(buf);
    return MP_OK;
  }
  return MP_ERROR_DECODE_INVALID_FORMAT;
}

mp_error_t mp_decode_stream_ext_len(mp_stream_t *stream, int8_t *out_type,
                                    uint32_t *out_len) {
  if (!out_type || !out_len)
    return MP_ERROR_BAD_ARG;
  uint8_t b;
  mp_error_t err = stream_read(stream, &b, 1);
  if (err != MP_OK)
    return err;

  char buf[4];
  uint32_t len = 0;
  switch (b) {
  case MP_TAG_FIXEXT1:
    len = 1;
    break;
  case MP_TAG_FIXEXT2:
    len = 2;
    break;
  case MP_TAG_FIXEXT4:
    len = 4;
    break;
  case MP_TAG_FIXEXT8:
    len = 8;
    break;
  case MP_TAG_FIXEXT16:
    len = 16;
    break;
  case MP_TAG_EXT8:
    if ((err = stream_read(stream, buf, 1)) != MP_OK)
      return err;
    len = (uint8_t)buf[0];
    break;
  case MP_TAG_EXT16:
    if ((err = stream_read(stream, buf, 2)) != MP_OK)
      return err;
    len = deserialize_be16(buf);
    break;
  case MP_TAG_EXT32:
    if ((err = stream_read(stream, buf, 4)) != MP_OK)
      return err;
    len = deserialize_be32(buf);
    break;
  default:
    return MP_ERROR_DECODE_INVALID_FORMAT;
  }

  uint8_t type_byte;
  if ((err = stream_read(stream, &type_byte, 1)) != MP_OK)
    return err;
  *out_type = (int8_t)type_byte;
  *out_len = len;

  return MP_OK;
}
