# Skip API

The Skip API allows you to quickly bypass MessagePack objects in a stream without allocating any memory to parse them into an Abstract Syntax Tree (AST). 

## Why use it?
- **Zero Allocation**: Skipping validates the structure of the MessagePack data and advances the stream, but avoids allocating memory for the objects it passes over.
- **Extreme Speed**: Bypassing large embedded payloads (like raw binaries, strings, or giant arrays) that you don't care about is orders of magnitude faster than fully decoding them.
- **Validation**: Functions like `mp_skip_memory` can be used to quickly validate if a block of binary memory contains a structurally sound MessagePack payload before attempting to allocate memory to parse it.

## How is it used?
The Skip API can be used directly on raw memory buffers for fast validation, or interleaved with standard decoding to selectively skip over fields in maps or arrays that you don't want to parse.

### Example: Selective Skipping
```c
#include <radikant-messagepack-c.h>
#include <stdio.h>

void selective_skip_example(const char *buffer, size_t size) {
    mp_stream_buffer_t dec_buffer;
    mp_stream_t stream;
    mp_stream_init_read(&stream, &dec_buffer, buffer, size);

    // Initialize a decoder with NO memory zone
    mp_decoder_t decoder;
    mp_decoder_init(&decoder, &stream, NULL); 

    // Imagine we are reading an array, but we only want the 3rd element
    mp_skip(&decoder); // Skip element 0 (no allocations!)
    mp_skip(&decoder); // Skip element 1 (no allocations!)
    
    // Now decode the 3rd element properly (requires a zone!)
    mp_zone_t zone;
    mp_zone_init(&zone, 4096);
    decoder.zone = &zone; // Attach zone
    
    mp_object_t ast;
    if (mp_decode(&decoder, &ast) == MP_OK) {
        printf("Successfully extracted the 3rd element!\n");
    }
    
    mp_zone_destroy(&zone);
}
```

### Example: Fast Validation
```c
#include <radikant-messagepack-c.h>
#include <stdio.h>

void validation_example(const char *buffer, size_t size) {
    // Quickly verifies if the entire buffer contains a valid MessagePack object
    // without allocating a single byte of memory.
    if (mp_skip_memory(buffer, size) == MP_OK) {
        printf("Payload is valid MessagePack!\n");
    } else {
        printf("Payload is corrupt or malformed!\n");
    }
}
```
