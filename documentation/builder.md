# Builder API

The Builder API (or DOM API) constructs an Abstract Syntax Tree (AST) of `mp_object_t` nodes in memory. 

## Why use it?
- **Highly Flexible**: You can query elements by index, modify values on the fly, and pass data structures around before serializing.
- **Easy to use**: Intuitive for building complex, nested JSON-like objects (Arrays and Maps).
- **Memory Dependent**: Requires an allocator (`mp_zone_t`) to dynamically allocate memory for every node in the tree. 

## Difference vs. Streaming API
Unlike the **Streaming API** (which writes raw bytes sequentially directly to a buffer), the Builder API constructs an in-memory Abstract Syntax Tree (AST). This makes the Builder API much more flexible—allowing you to arbitrarily query by index, modify data, and pass objects around—at the cost of requiring dynamic memory allocation (`mp_zone_t`) and a slightly higher performance overhead.

**Simplicity:** The Builder API is the **simpler** of the two for everyday use. Because it builds a tree, you can manipulate it just like standard JSON objects without having to manually track array/map lengths or strictly order your reads and writes.

## Example: Encoding

```c
#include <radikant-messagepack-c.h>

void encode_builder_example(char *buffer, size_t capacity, size_t *out_size) {
    mp_zone_t zone;
    mp_zone_init(&zone, 4096);

    // 1. Build the AST
    mp_object_t array;
    mp_build_array(&zone, &array, 3);
    mp_array_set_int(&array, 0, 42);
    mp_array_set_double(&array, 1, 3.14);
    mp_array_set_str(&array, 2, "Hello");

    // 2. Serialize the AST to a buffer
    mp_stream_buffer_t enc_buffer;
    mp_stream_t stream;
    mp_stream_init_write(&stream, &enc_buffer, false, buffer, capacity);
    
    mp_encode_object(&stream, &array);
    *out_size = enc_buffer.size;

    mp_zone_destroy(&zone);
}
```

## Example: Decoding

```c
#include <radikant-messagepack-c.h>
#include <stdio.h>

void decode_builder_example(const char *buffer, size_t size) {
    mp_zone_t zone;
    mp_zone_init(&zone, 4096);

    mp_stream_buffer_t dec_buffer;
    mp_stream_t stream;
    mp_stream_init_read(&stream, &dec_buffer, buffer, size);

    // Parse the entire stream into an AST
    mp_decoder_t decoder;
    mp_decoder_init(&decoder, &stream, &zone);

    mp_object_t ast;
    if (mp_decode(&decoder, &ast) == MP_OK) {
        int64_t val1;
        double val2;
        const char* val3_str;
        uint32_t val3_len;

        // Query the AST randomly
        mp_object_as_int(mp_array_get(&ast, 0), &val1);
        mp_object_as_double(mp_array_get(&ast, 1), &val2);
        mp_object_as_str(mp_array_get(&ast, 2), &val3_str, &val3_len);

        printf("Decoded: [%lld, %.2f, %.*s]\n", val1, val2, val3_len, val3_str);
    }

    mp_zone_destroy(&zone);
}
```
