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

    // Fast-path I/O for direct memory access (zero-call buffering)
    char* fast_ptr;
    size_t fast_left;
    size_t* fast_size_ptr; // Points to buffer->size (for write) or buffer->offset (for read)
};

// Memory Stream Context
typedef struct {
    char* data;
    size_t size;
    size_t capacity;
    size_t offset;
    bool is_dynamic;
} mp_stream_buffer_t;

// Initializes a stream for reading from a fixed memory buffer.
void mp_stream_init_read(mp_stream_t* stream, mp_stream_buffer_t* buffer, const char* data, size_t size);

// Initializes a stream for writing.
// If is_dynamic == true, the stream auto-allocates. buf and capacity are ignored.
// If is_dynamic == false, the stream writes to the fixed buf.
void mp_stream_init_write(mp_stream_t* stream, mp_stream_buffer_t* buffer, bool is_dynamic, char* buf, size_t capacity);

// Frees dynamically allocated memory in a dynamic memory stream context.
void mp_memory_stream_destroy(mp_stream_buffer_t* buffer);

// Initializes a stream for reading/writing using standard C FILE* I/O.
void mp_file_stream_init(mp_stream_t* stream, FILE* file);

#ifdef __cplusplus
}
#endif

#endif // RADIKANT_MESSAGEPACK_STREAM_H
