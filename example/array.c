#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <radikant-messagepack-c.h>

// ============================================================================
// 1. BUILDER / OBJECT API EXAMPLES
// ============================================================================
// The Builder API creates an AST (Abstract Syntax Tree) in memory. This is
// flexible but requires dynamic memory allocation (a Zone) and is more verbose.

int encode_builder_example(mp_zone_t *zone, mp_memory_stream_ctx_t *out_ctx) {
  printf("--- ENCODING VIA BUILDER API ---\n");
  mp_object_t array;
  mp_build_array(zone, &array, 4);

  mp_array_set_double(&array, 0, 98.6);
  mp_array_set_int(&array, 1, -10);
  mp_array_set_str(&array, 2, "Sunny");
  mp_array_set_bool(&array, 3, true);

  printf("[BUILDER ENCODE] Building AST array: [98.6, -10, \"Sunny\", true]\n");

  mp_stream_t enc_stream;
  mp_stream_init_write(&enc_stream, out_ctx, true, NULL, 0);

  mp_error_t err = mp_encode_object(&enc_stream, &array);
  if (err != MP_OK) {
    printf("Failed to encode object!\n");
    return 1;
  }

  printf("[BUILDER ENCODE] Successfully serialized to %zu bytes.\n\n",
         out_ctx->size);
  return 0;
}

int decode_builder_example(mp_zone_t *zone, const char* data, size_t size) {
  printf("--- DECODING VIA BUILDER API ---\n");
  mp_memory_stream_ctx_t dec_ctx;
  mp_stream_t dec_stream;
  mp_stream_init_read(&dec_stream, &dec_ctx, data, size);

  mp_decoder_t decoder;
  mp_decoder_init(&decoder, &dec_stream, zone);

  mp_object_t ast;
  mp_error_t err = mp_decode(&decoder, &ast);
  if (err != MP_OK) {
    printf("Failed to decode object! Error: %d\n", err);
    return 1;
  }

  if (ast.type == MP_TYPE_ARRAY && ast.via.array.size == 4) {
    double val1;
    int64_t val2;
    const char* val3_str;
    uint32_t val3_len;
    bool val4;

    mp_object_as_double(mp_array_get(&ast, 0), &val1);
    mp_object_as_int(mp_array_get(&ast, 1), &val2);
    mp_object_as_str(mp_array_get(&ast, 2), &val3_str, &val3_len);
    mp_object_as_bool(mp_array_get(&ast, 3), &val4);

    printf("[BUILDER DECODE] Index 0 (Double): %.1f\n", val1);
    printf("[BUILDER DECODE] Index 1 (Int)   : %lld\n", (long long)val2);
    printf("[BUILDER DECODE] Index 2 (String): %.*s\n", (int)val3_len, val3_str);
    printf("[BUILDER DECODE] Index 3 (Bool)  : %s\n", val4 ? "true" : "false");
  }
  printf("\n");
  return 0;
}

// ============================================================================
// 2. STREAMING API EXAMPLES
// ============================================================================
// The Streaming API pushes and pulls primitives directly to/from the buffer. 
// It requires NO memory allocator (No Zone!), making it incredibly fast.
// However, it is strictly sequential.

int encode_stream_example(mp_memory_stream_ctx_t *out_ctx) {
  printf("--- ENCODING VIA STREAMING API ---\n");
  mp_stream_t enc_stream;
  mp_stream_init_write(&enc_stream, out_ctx, true, NULL, 0);

  // Directly encode elements one by one without an AST
  mp_encode_array_len(&enc_stream, 4);
  mp_encode_double(&enc_stream, 98.6);
  mp_encode_int(&enc_stream, -10);
  
  const char* str = "Sunny";
  mp_encode_str(&enc_stream, str, strlen(str));
  mp_encode_bool(&enc_stream, true);

  printf("[STREAM ENCODE] Wrote array sequentially: [98.6, -10, \"Sunny\", true]\n");
  printf("[STREAM ENCODE] Successfully serialized to %zu bytes.\n\n", out_ctx->size);
  return 0;
}

int decode_stream_example(const char* data, size_t size) {
  printf("--- DECODING VIA STREAMING API ---\n");
  mp_memory_stream_ctx_t dec_ctx;
  mp_stream_t dec_stream;
  mp_stream_init_read(&dec_stream, &dec_ctx, data, size);

  uint32_t array_len;
  mp_decode_stream_array_len(&dec_stream, &array_len);
  
  if (array_len == 4) {
    double val1;
    mp_decode_stream_double(&dec_stream, &val1);
    
    int64_t val2;
    mp_decode_stream_int(&dec_stream, &val2);
    
    uint32_t str_len;
    mp_decode_stream_str_len(&dec_stream, &str_len);
    
    char str_buf[32] = {0};
    dec_stream.read(&dec_stream, str_buf, str_len); // Manually fetch string body
    
    bool val4;
    mp_decode_stream_bool(&dec_stream, &val4);
    
    printf("[STREAM DECODE] Index 0 (Double): %.1f\n", val1);
    printf("[STREAM DECODE] Index 1 (Int)   : %lld\n", (long long)val2);
    printf("[STREAM DECODE] Index 2 (String): %s\n", str_buf);
    printf("[STREAM DECODE] Index 3 (Bool)  : %s\n", val4 ? "true" : "false");
  }

  printf("\n");
  return 0;
}

int main() {
  printf("======================================\n");
  printf(" Radikant MessagePack-C Array Example\n");
  printf("======================================\n\n");

  mp_zone_t zone;
  mp_zone_init(&zone, 4096);

  // 1. Builder Test
  mp_memory_stream_ctx_t builder_ctx;
  if (encode_builder_example(&zone, &builder_ctx) == 0) {
    decode_builder_example(&zone, builder_ctx.data, builder_ctx.size);
    mp_memory_stream_destroy(&builder_ctx);
  }

  // 2. Stream Test
  mp_memory_stream_ctx_t stream_ctx;
  if (encode_stream_example(&stream_ctx) == 0) {
    decode_stream_example(stream_ctx.data, stream_ctx.size);
    mp_memory_stream_destroy(&stream_ctx);
  }

  mp_zone_destroy(&zone);
  return 0;
}
