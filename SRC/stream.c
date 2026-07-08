#include "stream.h"
#include <string.h>
#include <stdlib.h>

static mp_error_t mem_read(mp_stream_t* stream, void* buf, size_t count) {
    if (!stream || !stream->context || !buf) return MP_ERROR_BAD_ARG;
    if (count == 0) return MP_OK;
    mp_memory_stream_ctx_t* ctx = (mp_memory_stream_ctx_t*)stream->context;
    if (count > ctx->size - ctx->offset) return MP_ERROR_DECODE_INCOMPLETE;
    
    memcpy(buf, ctx->data + ctx->offset, count);
    ctx->offset += count;

    stream->fast_ptr = ctx->data + ctx->offset;
    stream->fast_left = ctx->size - ctx->offset;

    return MP_OK;
}

static mp_error_t mem_write(mp_stream_t* stream, const void* buf, size_t count) {
    if (!stream || !stream->context || (!buf && count > 0)) return MP_ERROR_BAD_ARG;
    if (count == 0) return MP_OK;

    mp_memory_stream_ctx_t* ctx = (mp_memory_stream_ctx_t*)stream->context;
    if (count > ctx->capacity - ctx->size) {
        if (!ctx->is_dynamic) return MP_ERROR_ENCODE_BUFFER_OVERFLOW;
        size_t new_cap = ctx->capacity == 0 ? 4096 : ctx->capacity * 2;
        while (new_cap - ctx->size < count) new_cap *= 2;
        char* new_data = (char*)realloc(ctx->data, new_cap);
        if (!new_data) return MP_ERROR_ENCODE_NOMEM;
        ctx->data = new_data;
        ctx->capacity = new_cap;
    }
    memcpy(ctx->data + ctx->size, buf, count);
    ctx->size += count;

    stream->fast_ptr = ctx->data + ctx->size;
    stream->fast_left = ctx->capacity - ctx->size;

    return MP_OK;
}

static mp_error_t mem_skip(mp_stream_t* stream, size_t count) {
    if (!stream || !stream->context) return MP_ERROR_BAD_ARG;
    if (count == 0) return MP_OK;
    mp_memory_stream_ctx_t* ctx = (mp_memory_stream_ctx_t*)stream->context;
    if (count > ctx->size - ctx->offset) return MP_ERROR_DECODE_INCOMPLETE;
    ctx->offset += count;

    stream->fast_ptr = ctx->data + ctx->offset;
    stream->fast_left = ctx->size - ctx->offset;

    return MP_OK;
}

void mp_stream_init_read(mp_stream_t* stream, mp_memory_stream_ctx_t* ctx, const char* data, size_t size) {
    if (!stream || !ctx) return;
    ctx->data = (char*)data; // Cast away const, only read will be used.
    ctx->size = size;
    ctx->capacity = size;
    ctx->offset = 0;
    ctx->is_dynamic = false;
    
    stream->context = ctx;
    stream->read = mem_read;
    stream->write = NULL;
    stream->skip = mem_skip;

    stream->fast_ptr = ctx->data;
    stream->fast_left = size;
    stream->fast_size_ptr = &ctx->offset;
}

void mp_stream_init_write(mp_stream_t* stream, mp_memory_stream_ctx_t* ctx, bool is_dynamic, char* buf, size_t capacity) {
    if (!stream || !ctx) return;
    if (is_dynamic) {
        ctx->data = NULL;
        ctx->size = 0;
        ctx->capacity = 0;
        ctx->offset = 0;
        ctx->is_dynamic = true;

        stream->fast_ptr = NULL;
        stream->fast_left = 0;
        stream->fast_size_ptr = &ctx->size;
    } else {
        ctx->data = buf;
        ctx->size = 0;
        ctx->capacity = capacity;
        ctx->offset = 0;
        ctx->is_dynamic = false;

        stream->fast_ptr = buf;
        stream->fast_left = capacity;
        stream->fast_size_ptr = &ctx->size;
    }

    stream->context = ctx;
    stream->read = NULL;
    stream->write = mem_write;
    stream->skip = NULL;
}

void mp_memory_stream_destroy(mp_memory_stream_ctx_t* ctx) {
    if (!ctx) return;
    if (ctx->is_dynamic && ctx->data) {
        free(ctx->data);
        ctx->data = NULL;
    }
    ctx->size = 0;
    ctx->capacity = 0;
    ctx->offset = 0;
}

static mp_error_t file_read(mp_stream_t* stream, void* buf, size_t count) {
    if (!stream || !stream->context || !buf) return MP_ERROR_BAD_ARG;
    if (count == 0) return MP_OK;
    FILE* file = (FILE*)stream->context;
    size_t bytes_read = fread(buf, 1, count, file);
    if (bytes_read < count) {
        if (ferror(file)) return MP_ERROR_DECODE_INVALID_FORMAT; // Or some IO error
        return MP_ERROR_DECODE_INCOMPLETE;
    }
    return MP_OK;
}

static mp_error_t file_write(mp_stream_t* stream, const void* buf, size_t count) {
    if (!stream || !stream->context || (!buf && count > 0)) return MP_ERROR_BAD_ARG;
    if (count == 0) return MP_OK;
    FILE* file = (FILE*)stream->context;
    size_t written = fwrite(buf, 1, count, file);
    if (written < count) return MP_ERROR_ENCODE_BUFFER_OVERFLOW; // IO write failure
    return MP_OK;
}

static mp_error_t file_skip(mp_stream_t* stream, size_t count) {
    if (!stream || !stream->context) return MP_ERROR_BAD_ARG;
    if (count == 0) return MP_OK;
    FILE* file = (FILE*)stream->context;
    if (fseek(file, count, SEEK_CUR) != 0) return MP_ERROR_DECODE_INCOMPLETE;
    return MP_OK;
}

void mp_file_stream_init(mp_stream_t* stream, FILE* file) {
    if (!stream || !file) return;
    stream->context = file;
    stream->read = file_read;
    stream->write = file_write;
    stream->skip = file_skip;

    stream->fast_ptr = NULL;
    stream->fast_left = 0;
    stream->fast_size_ptr = NULL;
}
