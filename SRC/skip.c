#include "skip.h"

#include "tools/endian.h"

static mp_error_t stream_skip(mp_decoder_t *decoder, size_t count) {
  if (count == 0)
    return MP_OK;
  mp_stream_t *stream = decoder->stream;
  if (!stream)
    return MP_ERROR_BAD_ARG;

  if (stream->fast_left >= count) {
    stream->fast_ptr += count;
    stream->fast_left -= count;
    if (stream->fast_size_ptr)
      *(stream->fast_size_ptr) += count;
    return MP_OK;
  }
  if (decoder->stream->skip) {
    return decoder->stream->skip(decoder->stream, count);
  } else if (decoder->stream->read) {
    // Fallback: read into dummy buffer if skip is not supported
    char dummy[256];
    size_t remaining = count;
    while (remaining > 0) {
      size_t to_read = remaining > sizeof(dummy) ? sizeof(dummy) : remaining;
      mp_error_t err = decoder->stream->read(decoder->stream, dummy, to_read);
      if (err != MP_OK)
        return err;
      remaining -= to_read;
    }
    return MP_OK;
  }
  return MP_ERROR_BAD_ARG;
}

static mp_error_t stream_read(mp_decoder_t *decoder, void *buf, size_t count) {
  if (count == 0)
    return MP_OK;
  mp_stream_t *stream = decoder->stream;
  if (!stream)
    return MP_ERROR_BAD_ARG;

  if (stream->fast_left >= count) {
    memcpy(buf, stream->fast_ptr, count);
    stream->fast_ptr += count;
    stream->fast_left -= count;
    if (stream->fast_size_ptr)
      *(stream->fast_size_ptr) += count;
    return MP_OK;
  }

  if (!stream->read)
    return MP_ERROR_BAD_ARG;
  return stream->read(stream, buf, count);
}

mp_error_t mp_skip(mp_decoder_t *decoder) {
  if (!decoder)
    return MP_ERROR_BAD_ARG;
  if (decoder->depth >= decoder->max_depth)
    return MP_ERROR_DECODE_DEPTH_EXCEEDED;

  uint8_t b;
  mp_error_t err = stream_read(decoder, &b, 1);
  if (err != MP_OK)
    return err;

  if (b <= MP_TAG_FIXINT_MAX) {
    return MP_OK;
  } else if (b >= MP_TAG_NEGFIXINT_MIN) {
    return MP_OK;
  } else if (b >= MP_TAG_FIXSTR_MIN && b <= MP_TAG_FIXSTR_MAX) {
    return stream_skip(decoder, b & 0x1f);
  } else if (b >= MP_TAG_FIXARRAY_MIN && b <= MP_TAG_FIXARRAY_MAX) {
    uint32_t len = b & 0x0f;
    for (uint32_t i = 0; i < len; ++i) {
      decoder->depth++;
      err = mp_skip(decoder);
      decoder->depth--;
      if (err != MP_OK)
        return err;
    }
    return MP_OK;
  } else if (b >= MP_TAG_FIXMAP_MIN && b <= MP_TAG_FIXMAP_MAX) {
    uint32_t len = b & 0x0f;
    for (uint32_t i = 0; i < len; ++i) {
      decoder->depth++;
      err = mp_skip(decoder);
      if (err == MP_OK)
        err = mp_skip(decoder);
      decoder->depth--;
      if (err != MP_OK)
        return err;
    }
    return MP_OK;
  }

  switch (b) {
  case MP_TAG_NIL:
  case MP_TAG_FALSE:
  case MP_TAG_TRUE:
    return MP_OK;
  case MP_TAG_FLOAT32:
  case MP_TAG_UINT32:
  case MP_TAG_INT32:
    return stream_skip(decoder, 4);
  case MP_TAG_FLOAT64:
  case MP_TAG_UINT64:
  case MP_TAG_INT64:
    return stream_skip(decoder, 8);
  case MP_TAG_UINT8:
  case MP_TAG_INT8:
    return stream_skip(decoder, 1);
  case MP_TAG_UINT16:
  case MP_TAG_INT16:
    return stream_skip(decoder, 2);

  case MP_TAG_STR8:
  case MP_TAG_BIN8: {
    char buf[1];
    err = stream_read(decoder, buf, 1);
    if (err != MP_OK)
      return MP_ERROR_DECODE_INCOMPLETE;
    return stream_skip(decoder, (uint8_t)buf[0]);
  }
  case MP_TAG_STR16:
  case MP_TAG_BIN16: {
    char buf[2];
    err = stream_read(decoder, buf, 2);
    if (err != MP_OK)
      return MP_ERROR_DECODE_INCOMPLETE;
    return stream_skip(decoder, deserialize_be16(buf));
  }
  case MP_TAG_STR32:
  case MP_TAG_BIN32: {
    char buf[4];
    err = stream_read(decoder, buf, 4);
    if (err != MP_OK)
      return MP_ERROR_DECODE_INCOMPLETE;
    return stream_skip(decoder, deserialize_be32(buf));
  }

  case MP_TAG_ARRAY16: {
    char buf[2];
    err = stream_read(decoder, buf, 2);
    if (err != MP_OK)
      return MP_ERROR_DECODE_INCOMPLETE;
    uint32_t len = deserialize_be16(buf);
    for (uint32_t i = 0; i < len; ++i) {
      decoder->depth++;
      err = mp_skip(decoder);
      decoder->depth--;
      if (err != MP_OK)
        return err;
    }
    return MP_OK;
  }
  case MP_TAG_ARRAY32: {
    char buf[4];
    err = stream_read(decoder, buf, 4);
    if (err != MP_OK)
      return MP_ERROR_DECODE_INCOMPLETE;
    uint32_t len = deserialize_be32(buf);
    for (uint32_t i = 0; i < len; ++i) {
      decoder->depth++;
      err = mp_skip(decoder);
      decoder->depth--;
      if (err != MP_OK)
        return err;
    }
    return MP_OK;
  }
  case MP_TAG_MAP16: {
    char buf[2];
    err = stream_read(decoder, buf, 2);
    if (err != MP_OK)
      return MP_ERROR_DECODE_INCOMPLETE;
    uint32_t len = deserialize_be16(buf);
    for (uint32_t i = 0; i < len; ++i) {
      decoder->depth++;
      err = mp_skip(decoder);
      if (err == MP_OK)
        err = mp_skip(decoder);
      decoder->depth--;
      if (err != MP_OK)
        return err;
    }
    return MP_OK;
  }
  case MP_TAG_MAP32: {
    char buf[4];
    err = stream_read(decoder, buf, 4);
    if (err != MP_OK)
      return MP_ERROR_DECODE_INCOMPLETE;
    uint32_t len = deserialize_be32(buf);
    for (uint32_t i = 0; i < len; ++i) {
      decoder->depth++;
      err = mp_skip(decoder);
      if (err == MP_OK)
        err = mp_skip(decoder);
      decoder->depth--;
      if (err != MP_OK)
        return err;
    }
    return MP_OK;
  }

  case MP_TAG_FIXEXT1:
    return stream_skip(decoder, 2);
  case MP_TAG_FIXEXT2:
    return stream_skip(decoder, 3);
  case MP_TAG_FIXEXT4:
    return stream_skip(decoder, 5);
  case MP_TAG_FIXEXT8:
    return stream_skip(decoder, 9);
  case MP_TAG_FIXEXT16:
    return stream_skip(decoder, 17);

  case MP_TAG_EXT8: {
    char buf[1];
    err = stream_read(decoder, buf, 1);
    if (err != MP_OK)
      return MP_ERROR_DECODE_INCOMPLETE;
    return stream_skip(decoder, 1 + (uint8_t)buf[0]);
  }
  case MP_TAG_EXT16: {
    char buf[2];
    err = stream_read(decoder, buf, 2);
    if (err != MP_OK)
      return MP_ERROR_DECODE_INCOMPLETE;
    return stream_skip(decoder, 1 + deserialize_be16(buf));
  }
  case MP_TAG_EXT32: {
    char buf[4];
    err = stream_read(decoder, buf, 4);
    if (err != MP_OK)
      return MP_ERROR_DECODE_INCOMPLETE;
    return stream_skip(decoder, 1 + deserialize_be32(buf));
  }

  default:
    return MP_ERROR_DECODE_INVALID_FORMAT;
  }
}

mp_error_t mp_skip_memory(const char *data, size_t size) {
  if (!data)
    return MP_ERROR_BAD_ARG;

  mp_stream_t stream;
  mp_memory_stream_ctx_t ctx;
  mp_stream_init_read(&stream, &ctx, data, size);

  mp_decoder_t decoder;
  mp_decoder_init(&decoder, &stream, NULL);

  return mp_skip(&decoder);
}