#ifndef RADIKANT_MESSAGEPACK_STREAM_H
#define RADIKANT_MESSAGEPACK_STREAM_H

#include "types.h"
#include "error.h"
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct mp_stream_s mp_stream_t;

typedef mp_error_t (*mp_stream_read_fn)(mp_stream_t* stream, void* buf, size_t count);
typedef mp_error_t (*mp_stream_write_fn)(mp_stream_t* stream, const void* buf, size_t count);
typedef mp_error_t (*mp_stream_skip_fn)(mp_stream_t* stream, size_t count);

struct mp_stream_s {
    void* context;
    mp_stream_read_fn read;
    mp_stream_write_fn write;
    mp_stream_skip_fn skip;
};

// Memory Stream Context
typedef struct {
    char* data;
    size_t size;
    size_t capacity;
    size_t offset;
    bool is_dynamic;
} mp_memory_stream_ctx_t;

// Initializes a stream for reading from a fixed memory buffer.
void mp_memory_stream_init_read(mp_stream_t* stream, mp_memory_stream_ctx_t* ctx, const char* data, size_t size);

// Initializes a stream for writing to a fixed memory buffer.
void mp_memory_stream_init_write_fixed(mp_stream_t* stream, mp_memory_stream_ctx_t* ctx, char* buf, size_t capacity);

// Initializes a stream for writing to a dynamically growing heap buffer.
void mp_memory_stream_init_write_dynamic(mp_stream_t* stream, mp_memory_stream_ctx_t* ctx);

// Frees dynamically allocated memory in a dynamic memory stream context.
void mp_memory_stream_destroy(mp_memory_stream_ctx_t* ctx);

// Initializes a stream for reading/writing using standard C FILE* I/O.
void mp_file_stream_init(mp_stream_t* stream, FILE* file);

#ifdef __cplusplus
}
#endif

#endif // RADIKANT_MESSAGEPACK_STREAM_H
