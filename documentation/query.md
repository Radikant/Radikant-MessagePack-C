# Query API

The Query API provides high-performance utility functions to search through MessagePack streams for specific elements without allocating memory for an Abstract Syntax Tree (AST).

## Why use it?
- **Zero Allocation**: Like the Skip API, the Query API never dynamically allocates memory. It parses keys into small stack buffers for fast comparisons and skips the rest.
- **Ease of Use**: Writing manual stream loops to search for map keys or array indices is tedious and error-prone. The Query API handles the jumping and type-checking for you cleanly.
- **Surgical Extraction**: Allows you to pluck a single nested value out of a multi-gigabyte payload almost instantly.

> [!WARNING]
> **Stream Consumption:** Because the Query API operates on raw streams, searching consumes bytes. If a query fails to find your key (`MP_ERROR_QUERY_NOT_FOUND`), the stream cannot magically rewind itself. The stream will be left positioned exactly at the end of the collection (map or array) that you were searching.

## Example: Searching a Map

```c
#include <radikant-messagepack-c.h>
#include <stdio.h>

void search_map_example(const char *buffer, size_t size) {
    mp_stream_buffer_t dec_buffer;
    mp_stream_t stream;
    mp_stream_init_read(&stream, &dec_buffer, buffer, size);

    // Initialize decoder without an allocator (Zero Allocation)
    mp_decoder_t decoder;
    mp_decoder_init(&decoder, &stream, NULL); 

    // Search the map for the key "employee_name"
    if (mp_query_map_key_str(&decoder, "employee_name") == MP_OK) {
        
        // The query found it! The decoder is perfectly positioned at the VALUE.
        // We can now safely read the value using the SAX/Streaming API.
        uint32_t str_len;
        if (mp_decode_stream_str_len(&decoder.stream, &str_len) == MP_OK) {
            char name[64] = {0};
            decoder.stream.read(&decoder.stream, name, str_len);
            printf("Found Employee: %s\n", name);
        }
    } else {
        printf("Employee Name key was not found!\n");
    }
}
```

## Example: Accessing an Array Index

```c
#include <radikant-messagepack-c.h>
#include <stdio.h>

void access_array_example(const char *buffer, size_t size) {
    mp_stream_buffer_t dec_buffer;
    mp_stream_t stream;
    mp_stream_init_read(&stream, &dec_buffer, buffer, size);

    mp_decoder_t decoder;
    mp_decoder_init(&decoder, &stream, NULL); 

    // Skip directly to the 5th element in the array (Index 4)
    if (mp_query_array_index(&decoder, 4) == MP_OK) {
        
        // The decoder is positioned at Index 4. 
        // We can now parse it into a full AST object if we want!
        mp_zone_t zone;
        mp_zone_init(&zone, 4096);
        decoder.zone = &zone; // Attach the memory allocator
        
        mp_object_t ast;
        if (mp_decode(&decoder, &ast) == MP_OK) {
            printf("Successfully parsed only the 5th element into an AST!\n");
        }
        
        mp_zone_destroy(&zone);
    }
}
```
