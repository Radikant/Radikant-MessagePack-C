#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Include the main MessagePack header
#include "../include/radikant-messagepack-c.h"

int main() {
    printf("======================================\n");
    printf(" Radikant MessagePack-C Example\n");
    printf("======================================\n\n");

    // ---------------------------------------------------------
    // 1. Initialize an Allocator (Zone)
    // ---------------------------------------------------------
    // A zone is an arena allocator that manages all the tiny
    // memory chunks required to build an Abstract Syntax Tree (AST).
    // When the zone is destroyed, all AST nodes are freed instantly.
    mp_zone_t zone;
    mp_zone_init(&zone, 4096);

    // ---------------------------------------------------------
    // 2. Build a MessagePack Object (AST)
    // ---------------------------------------------------------
    // Let's create a map like: {"name": "Radikant", "version": 1}
    mp_object_t map;
    map.type = MP_TYPE_MAP;
    map.via.map.size = 2;
    map.via.map.ptr = mp_zone_alloc(&zone, sizeof(mp_object_kv_t) * 2);

    // Key 1: "name"
    map.via.map.ptr[0].key = mp_zone_alloc(&zone, sizeof(mp_object_t));
    map.via.map.ptr[0].key->type = MP_TYPE_STR;
    map.via.map.ptr[0].key->via.str.ptr = "name";
    map.via.map.ptr[0].key->via.str.size = 4;

    // Value 1: "Radikant"
    map.via.map.ptr[0].val = mp_zone_alloc(&zone, sizeof(mp_object_t));
    map.via.map.ptr[0].val->type = MP_TYPE_STR;
    map.via.map.ptr[0].val->via.str.ptr = "Radikant";
    map.via.map.ptr[0].val->via.str.size = 8;

    // Key 2: "version"
    map.via.map.ptr[1].key = mp_zone_alloc(&zone, sizeof(mp_object_t));
    map.via.map.ptr[1].key->type = MP_TYPE_STR;
    map.via.map.ptr[1].key->via.str.ptr = "version";
    map.via.map.ptr[1].key->via.str.size = 7;

    // Value 2: 1
    map.via.map.ptr[1].val = mp_zone_alloc(&zone, sizeof(mp_object_t));
    map.via.map.ptr[1].val->type = MP_TYPE_POSITIVE_INTEGER;
    map.via.map.ptr[1].val->via.u64 = 1;

    printf("[ENCODE] Building AST map: {\"name\": \"Radikant\", \"version\": 1}\n");

    // ---------------------------------------------------------
    // 3. Encode the Object to a Memory Stream
    // ---------------------------------------------------------
    mp_memory_stream_ctx_t enc_ctx;
    mp_stream_t enc_stream;
    
    // Using a dynamic stream automatically handles memory reallocation!
    mp_memory_stream_init_write_dynamic(&enc_stream, &enc_ctx);
    
    // Perform the encoding
    mp_error_t err = mp_encode_object(&enc_stream, &map);
    if (err != MP_OK) {
        printf("Failed to encode object!\n");
        return 1;
    }
    
    printf("[ENCODE] Successfully serialized to %zu bytes of binary data.\n\n", enc_ctx.size);

    // ---------------------------------------------------------
    // 4. Decode the Binary Data back to an AST
    // ---------------------------------------------------------
    mp_memory_stream_ctx_t dec_ctx;
    mp_stream_t dec_stream;
    
    // Read from the dynamically allocated buffer
    mp_memory_stream_init_read(&dec_stream, &dec_ctx, enc_ctx.data, enc_ctx.size);

    mp_decoder_t decoder;
    mp_decoder_init(&decoder, &dec_stream, &zone);

    mp_object_t decoded_ast;
    err = mp_decode(&decoder, &decoded_ast);
    if (err != MP_OK) {
        printf("Failed to decode object! Error: %d\n", err);
        return 1;
    }

    printf("[DECODE] Successfully parsed binary data back into an AST.\n");
    
    // ---------------------------------------------------------
    // 5. Verify the Decoded Data
    // ---------------------------------------------------------
    if (decoded_ast.type == MP_TYPE_MAP && decoded_ast.via.map.size == 2) {
        mp_object_t *val1 = decoded_ast.via.map.ptr[0].val;
        mp_object_t *val2 = decoded_ast.via.map.ptr[1].val;
        
        printf("[VERIFY] Value 1: %.*s\n", (int)val1->via.str.size, val1->via.str.ptr);
        printf("[VERIFY] Value 2: %llu\n", (unsigned long long)val2->via.u64);
    }

    printf("\n======================================\n");

    // ---------------------------------------------------------
    // 6. Cleanup
    // ---------------------------------------------------------
    // Destroying the zone frees all memory tied to `map` AND `decoded_ast` instantly!
    mp_zone_destroy(&zone);
    mp_memory_stream_destroy(&enc_ctx); // Free the dynamic stream buffer

    return 0;
}