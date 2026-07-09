# Streaming API

The Streaming API reads and writes MessagePack primitives sequentially directly to/from binary memory buffers.

## Why use it?
- **Zero Allocation**: Encoding requires no dynamic memory allocation (`mp_zone_t` is not needed).
- **Maximum Performance**: Blazingly fast, ideal for embedded systems or high-performance networking.
- **Strictly Sequential**: Data must be encoded and decoded in the exact order it appears. You cannot query by index directly, but you *can* skip unwanted elements using the `mp_skip` API to fast-forward through the stream.

## Difference vs. Builder API
Unlike the **Builder API** (which dynamically builds an entire Abstract Syntax Tree in memory before encoding), the Streaming API operates directly on raw buffers. This means the Streaming API uses almost zero RAM and is significantly faster, but lacks the flexibility of modifying, reordering, or querying data by index before it is written.

**Simplicity:** The Builder API is generally simpler to use for complex or nested data because it behaves like standard JSON objects. The Streaming API requires you to manually track structure (like array/map lengths) and read/write strictly in order.

## Example: Encoding

```c
#include <radikant-messagepack-c.h>

void encode_stream_example(char *buffer, size_t capacity, size_t *out_size) {
    mp_stream_buffer_t enc_buffer;
    mp_stream_t stream;
    
    // Initialize stream against a pre-allocated stack buffer
    mp_stream_init_write(&stream, &enc_buffer, false, buffer, capacity);

    // Write elements sequentially
    mp_encode_array_len(&stream, 3);
    mp_encode_int(&stream, 42);
    mp_encode_double(&stream, 3.14);
    
    const char *str = "Hello";
    mp_encode_str(&stream, str, strlen(str));

    *out_size = enc_buffer.size;
}
```

## Example: Decoding

```c
#include <radikant-messagepack-c.h>
#include <stdio.h>

void decode_stream_example(const char *buffer, size_t size) {
    mp_stream_buffer_t dec_buffer;
    mp_stream_t stream;
    
    mp_stream_init_read(&stream, &dec_buffer, buffer, size);

    // Read elements sequentially
    uint32_t array_len;
    mp_decode_stream_array_len(&stream, &array_len);

    int64_t val1;
    mp_decode_stream_int(&stream, &val1);

    double val2;
    mp_decode_stream_double(&stream, &val2);

    uint32_t str_len;
    mp_decode_stream_str_len(&stream, &str_len);
    
    char str_buf[32] = {0};
    stream.read(&stream, str_buf, str_len); // Fetch string body

    printf("Decoded: [%lld, %.2f, %s]\n", val1, val2, str_buf);
}
```

## Example: Skipping Elements (`mp_skip`)

The Streaming API becomes incredibly powerful when combined with the **Skip API** (`SRC/skip.h`). If you encounter an element you don't care about, you can completely jump over it in a zero-allocation way.

```c
#include <radikant-messagepack-c.h>

void skip_stream_example(mp_decoder_t *decoder) {
    uint32_t map_size;
    mp_decode_stream_map_len(decoder->stream, &map_size);

    for (uint32_t i = 0; i < map_size; i++) {
        // Read Key
        uint32_t key_len;
        mp_decode_stream_str_len(decoder->stream, &key_len);
        char key[128];
        decoder->stream->read(decoder->stream, key, key_len);
        key[key_len] = '\0';
        
        if (strcmp(key, "target_key") == 0) {
            // We found our target! Decode it.
            int64_t val;
            mp_decode_stream_int(decoder->stream, &val);
        } else {
            // Not our target! Swiftly skip over the value.
            // If this value is a massive nested Array, mp_skip 
            // will automatically traverse and skip all of it!
            mp_skip(decoder);
        }
    }
}
```

By mixing `mp_decode_stream_*` and `mp_skip()`, you can construct blazing fast custom extractors that parse massive MessagePack files with essentially zero heap allocations.
