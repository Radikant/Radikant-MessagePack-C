#include "encoder.h"
#include "object.h"
#include <stdlib.h>
#include <string.h>

#include "tools/endian.h"

static inline mp_error_t mp_encode_bytes(mp_stream_t *stream, const void *data,
                                         size_t len) {
  if (!stream)
    return MP_ERROR_BAD_ARG;
  if (stream->fast_left >= len) {
    memcpy(stream->fast_ptr, data, len);
    stream->fast_ptr += len;
    stream->fast_left -= len;
    if (stream->fast_size_ptr)
      *(stream->fast_size_ptr) += len;
    return MP_OK;
  }
  if (!stream->write)
    return MP_ERROR_BAD_ARG;
  return stream->write(stream, data, len);
}

mp_error_t mp_encode_nil(mp_stream_t *stream) {
  char d = (char)MP_TAG_NIL;
  return mp_encode_bytes(stream, &d, sizeof(d));
}

mp_error_t mp_encode_bool(mp_stream_t *stream, bool val) {
  char d = val ? (char)MP_TAG_TRUE : (char)MP_TAG_FALSE;
  return mp_encode_bytes(stream, &d, sizeof(d));
}

mp_error_t mp_encode_int(mp_stream_t *stream, int64_t val) {
  if (val >= -32 && val <= 127) {
    char d = (char)val;
    return mp_encode_bytes(stream, &d, sizeof(d));
  } else if (val >= -128 && val < 0) {
    char d[2] = {(char)MP_TAG_INT8, (char)val};
    return mp_encode_bytes(stream, d, sizeof(d));
  } else if (val >= -32768 && val < 0) {
    char d[3] = {(char)MP_TAG_INT16};
    serialize_be16(d + 1, (uint16_t)val);
    return mp_encode_bytes(stream, d, sizeof(d));
  } else if (val >= -2147483648LL && val < 0) {
    char d[5] = {(char)MP_TAG_INT32};
    serialize_be32(d + 1, (uint32_t)val);
    return mp_encode_bytes(stream, d, sizeof(d));
  } else {
    char d[9] = {(char)MP_TAG_INT64};
    serialize_be64(d + 1, (uint64_t)val);
    return mp_encode_bytes(stream, d, sizeof(d));
  }
}

mp_error_t mp_encode_uint(mp_stream_t *stream, uint64_t val) {
  if (val <= 127) {
    char d = (char)val;
    return mp_encode_bytes(stream, &d, sizeof(d));
  } else if (val <= 255) {
    char d[2] = {(char)MP_TAG_UINT8, (char)val};
    return mp_encode_bytes(stream, d, sizeof(d));
  } else if (val <= 65535) {
    char d[3] = {(char)MP_TAG_UINT16};
    serialize_be16(d + 1, (uint16_t)val);
    return mp_encode_bytes(stream, d, sizeof(d));
  } else if (val <= 4294967295ULL) {
    char d[5] = {(char)MP_TAG_UINT32};
    serialize_be32(d + 1, (uint32_t)val);
    return mp_encode_bytes(stream, d, sizeof(d));
  } else {
    char d[9] = {(char)MP_TAG_UINT64};
    serialize_be64(d + 1, val);
    return mp_encode_bytes(stream, d, sizeof(d));
  }
}

mp_error_t mp_encode_float(mp_stream_t *stream, float val) {
  union {
    float f;
    uint32_t u;
  } u = {val};
  char d[5] = {(char)MP_TAG_FLOAT32};
  serialize_be32(d + 1, u.u);
  return mp_encode_bytes(stream, d, sizeof(d));
}

mp_error_t mp_encode_double(mp_stream_t *stream, double val) {
  union {
    double d;
    uint64_t u;
  } u = {val};
  char d[9] = {(char)MP_TAG_FLOAT64};
  serialize_be64(d + 1, u.u);
  return mp_encode_bytes(stream, d, sizeof(d));
}

mp_error_t mp_encode_str_len(mp_stream_t *stream, uint32_t len) {
  if (len <= 31) {
    char d = (char)(MP_TAG_FIXSTR_MIN | len);
    return mp_encode_bytes(stream, &d, sizeof(d));
  } else if (len <= 255) {
    char d[2] = {(char)MP_TAG_STR8, (char)len};
    return mp_encode_bytes(stream, d, sizeof(d));
  } else if (len <= 65535) {
    char d[3] = {(char)MP_TAG_STR16};
    serialize_be16(d + 1, (uint16_t)len);
    return mp_encode_bytes(stream, d, sizeof(d));
  } else {
    char d[5] = {(char)MP_TAG_STR32};
    serialize_be32(d + 1, len);
    return mp_encode_bytes(stream, d, sizeof(d));
  }
}

mp_error_t mp_encode_str(mp_stream_t *stream, const char *str, uint32_t len) {
  if (!str && len > 0)
    return MP_ERROR_ENCODE_NULL_PAYLOAD;
  mp_error_t err = mp_encode_str_len(stream, len);
  if (err != MP_OK)
    return err;
  if (len > 0)
    return mp_encode_bytes(stream, str, len);
  return MP_OK;
}

mp_error_t mp_encode_bin_len(mp_stream_t *stream, uint32_t len) {
  if (len <= 255) {
    char d[2] = {(char)MP_TAG_BIN8, (char)len};
    return mp_encode_bytes(stream, d, sizeof(d));
  } else if (len <= 65535) {
    char d[3] = {(char)MP_TAG_BIN16};
    serialize_be16(d + 1, (uint16_t)len);
    return mp_encode_bytes(stream, d, sizeof(d));
  } else {
    char d[5] = {(char)MP_TAG_BIN32};
    serialize_be32(d + 1, len);
    return mp_encode_bytes(stream, d, sizeof(d));
  }
}

mp_error_t mp_encode_bin(mp_stream_t *stream, const char *bin, uint32_t len) {
  if (!bin && len > 0)
    return MP_ERROR_ENCODE_NULL_PAYLOAD;
  mp_error_t err = mp_encode_bin_len(stream, len);
  if (err != MP_OK)
    return err;
  if (len > 0)
    return mp_encode_bytes(stream, bin, len);
  return MP_OK;
}

mp_error_t mp_encode_array_len(mp_stream_t *stream, uint32_t len) {
  if (len <= 15) {
    char d = (char)(MP_TAG_FIXARRAY_MIN | len);
    return mp_encode_bytes(stream, &d, sizeof(d));
  } else if (len <= 65535) {
    char d[3] = {(char)MP_TAG_ARRAY16};
    serialize_be16(d + 1, (uint16_t)len);
    return mp_encode_bytes(stream, d, sizeof(d));
  } else {
    char d[5] = {(char)MP_TAG_ARRAY32};
    serialize_be32(d + 1, len);
    return mp_encode_bytes(stream, d, sizeof(d));
  }
}

mp_error_t mp_encode_map_len(mp_stream_t *stream, uint32_t len) {
  if (len <= 15) {
    char d = (char)(MP_TAG_FIXMAP_MIN | len);
    return mp_encode_bytes(stream, &d, sizeof(d));
  } else if (len <= 65535) {
    char d[3] = {(char)MP_TAG_MAP16};
    serialize_be16(d + 1, (uint16_t)len);
    return mp_encode_bytes(stream, d, sizeof(d));
  } else {
    char d[5] = {(char)MP_TAG_MAP32};
    serialize_be32(d + 1, len);
    return mp_encode_bytes(stream, d, sizeof(d));
  }
}

mp_error_t mp_encode_ext_len(mp_stream_t *stream, int8_t type, uint32_t len) {
  // Dont change this, the compiler will already handle this optimization.
  if (len == 1) {
    char d[2] = {(char)MP_TAG_FIXEXT1, type};
    return mp_encode_bytes(stream, d, 2);
  } else if (len == 2) {
    char d[2] = {(char)MP_TAG_FIXEXT2, type};
    return mp_encode_bytes(stream, d, 2);
  } else if (len == 4) {
    char d[2] = {(char)MP_TAG_FIXEXT4, type};
    return mp_encode_bytes(stream, d, 2);
  } else if (len == 8) {
    char d[2] = {(char)MP_TAG_FIXEXT8, type};
    return mp_encode_bytes(stream, d, 2);
  } else if (len == 16) {
    char d[2] = {(char)MP_TAG_FIXEXT16, type};
    return mp_encode_bytes(stream, d, 2);
  } else if (len <= 255) {
    char d[3] = {(char)MP_TAG_EXT8, (char)len, type};
    return mp_encode_bytes(stream, d, 3);
  } else if (len <= 65535) {
    char d[4] = {(char)MP_TAG_EXT16};
    serialize_be16(d + 1, (uint16_t)len);
    d[3] = type;
    return mp_encode_bytes(stream, d, 4);
  } else {
    char d[6] = {(char)MP_TAG_EXT32};
    serialize_be32(d + 1, len);
    d[5] = type;
    return mp_encode_bytes(stream, d, 6);
  }
}

mp_error_t mp_encode_ext(mp_stream_t *stream, int8_t type, const char *data,
                         uint32_t len) {
  if (!data && len > 0)
    return MP_ERROR_ENCODE_NULL_PAYLOAD;
  mp_error_t err = mp_encode_ext_len(stream, type, len);
  if (err != MP_OK)
    return err;
  if (len > 0)
    return mp_encode_bytes(stream, data, len);
  return MP_OK;
}

mp_error_t mp_encode_object(mp_stream_t *stream, const mp_object_t *obj) {
  if (!obj)
    return MP_ERROR_BAD_ARG;
  switch (obj->type) {
  case MP_TYPE_NIL:
    return mp_encode_nil(stream);
  case MP_TYPE_BOOLEAN:
    return mp_encode_bool(stream, obj->via.boolean);
  case MP_TYPE_POSITIVE_INTEGER:
    return mp_encode_uint(stream, obj->via.u64);
  case MP_TYPE_NEGATIVE_INTEGER:
    return mp_encode_int(stream, obj->via.i64);
  case MP_TYPE_FLOAT32:
    return mp_encode_float(stream, obj->via.f32);
  case MP_TYPE_FLOAT64:
    return mp_encode_double(stream, obj->via.f64);
  case MP_TYPE_STR:
    return mp_encode_str(stream, obj->via.str.ptr, obj->via.str.size);
  case MP_TYPE_BIN:
    return mp_encode_bin(stream, obj->via.bin.ptr, obj->via.bin.size);
  case MP_TYPE_ARRAY: {
    mp_error_t err = mp_encode_array_len(stream, obj->via.array.size);
    if (err != MP_OK)
      return err;
    for (uint32_t i = 0; i < obj->via.array.size; i++) {
      err = mp_encode_object(stream, &obj->via.array.ptr[i]);
      if (err != MP_OK)
        return err;
    }
    return MP_OK;
  }
  case MP_TYPE_MAP: {
    mp_error_t err = mp_encode_map_len(stream, obj->via.map.size);
    if (err != MP_OK)
      return err;
    for (uint32_t i = 0; i < obj->via.map.size; i++) {
      err = mp_encode_object(stream, &obj->via.map.ptr[i].key);
      if (err != MP_OK)
        return err;
      err = mp_encode_object(stream, &obj->via.map.ptr[i].val);
      if (err != MP_OK)
        return err;
    }
    return MP_OK;
  }
  case MP_TYPE_EXT:
    return mp_encode_ext(stream, obj->via.ext.type, obj->via.ext.ptr,
                         obj->via.ext.size);
  default:
    return MP_ERROR_ENCODE_UNSUPPORTED_TYPE;
  }
}

mp_error_t mp_serialize_memory(const mp_object_t *obj, char **out_data,
                               size_t *out_size) {
  if (!obj || !out_data || !out_size)
    return MP_ERROR_BAD_ARG;

  mp_stream_buffer_t buffer;
  mp_stream_t stream;
  mp_stream_init_write(&stream, &buffer, true, NULL, 0);

  mp_error_t err = mp_encode_object(&stream, obj);
  if (err == MP_OK) {
    *out_data = buffer.data;
    *out_size = buffer.size;
  } else {
    mp_memory_stream_destroy(&buffer);
    *out_data = NULL;
    *out_size = 0;
  }
  return err;
}
