#include <stdio.h>
#include <string.h>

#include "../include/radikant-messagepack-c.h"

int main() {
    printf("--- Radikant MessagePack-C Simple Example ---\n\n");

    // ---------------------------------------------------------
    // 1. ENCODING (No Zone Required!)
    // ---------------------------------------------------------
    // If you use the Streaming API, you can encode data directly
    // into a buffer without ever allocating an AST or using a Zone.
    char buffer[256];
    mp_memory_stream_ctx_t enc_ctx;
    mp_stream_t enc_stream;
    mp_memory_stream_init_write_fixed(&enc_stream, &enc_ctx, buffer, sizeof(buffer));

    // Encode an Array: [42, 99, "Hello"]
    mp_encode_array_len(&enc_stream, 3);
    mp_encode_int(&enc_stream, 42);
    mp_encode_int(&enc_stream, 99);
    
    const char *str = "Hello";
    mp_encode_str(&enc_stream, str, strlen(str));

    printf("[ENCODED] Wrote %zu bytes directly to stack buffer.\n", enc_ctx.size);

    // ---------------------------------------------------------
    // 2. DECODING (Zone Required)
    // ---------------------------------------------------------
    // Unlike encoding, decoding builds a recursive AST (Abstract Syntax Tree).
    // The library MUST have an allocator to instantiate the Array structures
    // and copy the String bytes safely into memory. Therefore, a Zone is required.
    mp_zone_t zone;
    mp_zone_init(&zone, 4096);

    mp_memory_stream_ctx_t dec_ctx;
    mp_stream_t dec_stream;
    mp_memory_stream_init_read(&dec_stream, &dec_ctx, buffer, enc_ctx.size);

    mp_decoder_t decoder;
    mp_decoder_init(&decoder, &dec_stream, &zone);

    mp_object_t ast;
    mp_decode(&decoder, &ast);

    // ---------------------------------------------------------
    // 3. Verification
    // ---------------------------------------------------------
    if (ast.type == MP_TYPE_ARRAY && ast.via.array.size == 3) {
        int64_t val1 = ast.via.array.ptr[0].via.u64; // Since 42 is positive, it reads as u64
        int64_t val2 = ast.via.array.ptr[1].via.u64; // 99 is also positive
        mp_object_str_t parsed_str = ast.via.array.ptr[2].via.str;

        printf("[DECODED] Int 1: %lld\n", val1);
        printf("[DECODED] Int 2: %lld\n", val2);
        printf("[DECODED] String : %.*s\n", (int)parsed_str.size, parsed_str.ptr);
    }

    // Free the decoded AST instantly
    mp_zone_destroy(&zone);

    return 0;
}