#include "stream.h"
#include <stdlib.h>
#include <string.h>

static mp_error_t mem_read(mp_stream_t *stream, void *buf, size_t count) {
  if (!stream || !stream->context || !buf)
    return MP_ERROR_BAD_ARG;
  if (count == 0)
    return MP_OK;
  mp_stream_buffer_t *buffer = (mp_stream_buffer_t *)stream->context;
  if (count > buffer->size - buffer->offset)
    return MP_ERROR_DECODE_INCOMPLETE;

  memcpy(buf, buffer->data + buffer->offset, count);
  buffer->offset += count;

  stream->fast_ptr = buffer->data + buffer->offset;
  stream->fast_left = buffer->size - buffer->offset;

  return MP_OK;
}

static mp_error_t mem_write(mp_stream_t *stream, const void *buf,
                            size_t count) {
  if (!stream || !stream->context || (!buf && count > 0))
    return MP_ERROR_BAD_ARG;
  if (count == 0)
    return MP_OK;

  mp_stream_buffer_t *buffer = (mp_stream_buffer_t *)stream->context;
  if (count > buffer->capacity - buffer->size) {
    if (!buffer->is_dynamic)
      return MP_ERROR_ENCODE_BUFFER_OVERFLOW;
    size_t new_cap = buffer->capacity == 0 ? 4096 : buffer->capacity * 2;
    while (new_cap - buffer->size < count)
      new_cap *= 2;
    char *new_data = (char *)realloc(buffer->data, new_cap);
    if (!new_data)
      return MP_ERROR_ENCODE_NOMEM;
    buffer->data = new_data;
    buffer->capacity = new_cap;
  }
  memcpy(buffer->data + buffer->size, buf, count);
  buffer->size += count;

  stream->fast_ptr = buffer->data + buffer->size;
  stream->fast_left = buffer->capacity - buffer->size;

  return MP_OK;
}

static mp_error_t mem_skip(mp_stream_t *stream, size_t count) {
  if (!stream || !stream->context)
    return MP_ERROR_BAD_ARG;
  if (count == 0)
    return MP_OK;
  mp_stream_buffer_t *buffer = (mp_stream_buffer_t *)stream->context;
  if (count > buffer->size - buffer->offset)
    return MP_ERROR_DECODE_INCOMPLETE;
  buffer->offset += count;

  stream->fast_ptr = buffer->data + buffer->offset;
  stream->fast_left = buffer->size - buffer->offset;

  return MP_OK;
}

mp_error_t mp_stream_init_read(mp_stream_t *stream, mp_stream_buffer_t *buffer,
                               const char *data, size_t size) {
  if (!stream || !buffer)
    return MP_ERROR_STREAM_BAD_ARG;
  buffer->data = (char *)data; // Cast away const, only read will be used.
  buffer->size = size;
  buffer->capacity = size;
  buffer->offset = 0;
  buffer->is_dynamic = false;

  stream->context = buffer;
  stream->read = mem_read;
  stream->write = NULL;
  stream->skip = mem_skip;

  stream->fast_ptr = buffer->data;
  stream->fast_left = size;
  stream->fast_size_ptr = &buffer->offset;

  return MP_OK;
}

mp_error_t mp_stream_init_write(mp_stream_t *stream, mp_stream_buffer_t *buffer,
                                bool is_dynamic, char *buf, size_t capacity) {
  if (!stream || !buffer)
    return MP_ERROR_STREAM_BAD_ARG;
  if (is_dynamic) {
    buffer->data = NULL;
    buffer->size = 0;
    buffer->capacity = 0;
    buffer->offset = 0;
    buffer->is_dynamic = true;

    stream->fast_ptr = NULL;
    stream->fast_left = 0;
    stream->fast_size_ptr = &buffer->size;
  } else {
    buffer->data = buf;
    buffer->size = 0;
    buffer->capacity = capacity;
    buffer->offset = 0;
    buffer->is_dynamic = false;

    stream->fast_ptr = buf;
    stream->fast_left = capacity;
    stream->fast_size_ptr = &buffer->size;
  }

  stream->context = buffer;
  stream->read = NULL;
  stream->write = mem_write;
  stream->skip = NULL;

  return MP_OK;
}

void mp_memory_stream_destroy(mp_stream_buffer_t *buffer) {
  if (!buffer)
    return;
  if (buffer->is_dynamic && buffer->data) {
    free(buffer->data);
    buffer->data = NULL;
  }
  buffer->size = 0;
  buffer->capacity = 0;
  buffer->offset = 0;
}

static mp_error_t file_read(mp_stream_t *stream, void *buf, size_t count) {
  if (!stream || !stream->context || !buf)
    return MP_ERROR_BAD_ARG;
  if (count == 0)
    return MP_OK;
  FILE *file = (FILE *)stream->context;
  size_t bytes_read = fread(buf, 1, count, file);
  if (bytes_read < count) {
    if (ferror(file))
      return MP_ERROR_DECODE_INVALID_FORMAT; // Or some IO error
    return MP_ERROR_DECODE_INCOMPLETE;
  }
  return MP_OK;
}

static mp_error_t file_write(mp_stream_t *stream, const void *buf,
                             size_t count) {
  if (!stream || !stream->context || (!buf && count > 0))
    return MP_ERROR_BAD_ARG;
  if (count == 0)
    return MP_OK;
  FILE *file = (FILE *)stream->context;
  size_t written = fwrite(buf, 1, count, file);
  if (written < count)
    return MP_ERROR_ENCODE_BUFFER_OVERFLOW; // IO write failure
  return MP_OK;
}

static mp_error_t file_skip(mp_stream_t *stream, size_t count) {
  if (!stream || !stream->context)
    return MP_ERROR_BAD_ARG;
  if (count == 0)
    return MP_OK;
  FILE *file = (FILE *)stream->context;
  if (fseek(file, count, SEEK_CUR) != 0)
    return MP_ERROR_DECODE_INCOMPLETE;
  return MP_OK;
}

mp_error_t mp_file_stream_init(mp_stream_t *stream, FILE *file) {
  if (!stream || !file)
    return MP_ERROR_STREAM_BAD_ARG;
  stream->context = file;
  stream->read = file_read;
  stream->write = file_write;
  stream->skip = file_skip;

  stream->fast_ptr = NULL;
  stream->fast_left = 0;
  stream->fast_size_ptr = NULL;

  return MP_OK;
}