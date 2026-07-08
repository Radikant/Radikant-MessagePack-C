#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <radikant-messagepack-c.h>

// ============================================================================
// 1. BUILDER / OBJECT API EXAMPLES
// ============================================================================
// The Builder API creates an AST (Abstract Syntax Tree) in memory. This is
// flexible but requires dynamic memory allocation (a Zone) and is more verbose.

int encode_builder_example(mp_zone_t *zone, mp_stream_buffer_t *out_buffer) {
  printf("--- ENCODING VIA BUILDER API ---\n");
  mp_object_t array;
  mp_build_array(zone, &array, 4);

  mp_array_set_double(&array, 0, 98.6);
  mp_array_set_int(&array, 1, -10);
  mp_array_set_str(&array, 2, "Sunny");
  mp_array_set_bool(&array, 3, true);

  printf("[BUILDER ENCODE] Building AST array: [98.6, -10, \"Sunny\", true]\n");

  mp_stream_t enc_stream;
  mp_stream_init_write(&enc_stream, out_buffer, true, NULL, 0);

  mp_error_t err = mp_encode_object(&enc_stream, &array);
  if (err != MP_OK) {
    printf("Failed to encode object!\n");
    return 1;
  }

  printf("[BUILDER ENCODE] Successfully serialized to %zu bytes.\n\n",
         out_buffer->size);
  return 0;
}

int decode_builder_example(mp_zone_t *zone, const char *data, size_t size) {
  printf("--- DECODING VIA BUILDER API ---\n");
  mp_stream_buffer_t dec_buffer;
  mp_stream_t dec_stream;
  mp_stream_init_read(&dec_stream, &dec_buffer, data, size);

  mp_decoder_t decoder;
  mp_decoder_init(&decoder, &dec_stream, zone);

  mp_object_t ast;
  mp_error_t err = mp_decode(&decoder, &ast);
  if (err != MP_OK) {
    printf("Failed to decode object! Error: %d\n", err);
    return 1;
  }

  if (ast.type == MP_TYPE_ARRAY && ast.via.array.size == 4) {
    double val_double;
    int64_t val_int64;
    const char *val_char;
    uint32_t val_char_len;
    bool val_bool;

    mp_object_as_double(mp_array_get(&ast, 0), &val_double);
    mp_object_as_int(mp_array_get(&ast, 1), &val_int64);
    mp_object_as_str(mp_array_get(&ast, 2), &val_char, &val_char_len);
    mp_object_as_bool(mp_array_get(&ast, 3), &val_bool);

    printf("[BUILDER DECODE] Index 0 (Double): %.1f\n", val_double);
    printf("[BUILDER DECODE] Index 1 (Int)   : %lld\n", (long long)val_int64);
    printf("[BUILDER DECODE] Index 2 (String): %.*s\n", (int)val_char_len,
           val_char);
    printf("[BUILDER DECODE] Index 3 (Bool)  : %s\n", val_bool ? "true" : "false");
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

int encode_stream_example(mp_stream_buffer_t *out_buffer) {
  printf("--- ENCODING VIA STREAMING API ---\n");
  mp_stream_t enc_stream;
  mp_stream_init_write(&enc_stream, out_buffer, true, NULL, 0);

  // Directly encode elements one by one without an AST
  mp_encode_array_len(&enc_stream, 4);
  mp_encode_double(&enc_stream, 98.6);
  mp_encode_int(&enc_stream, -10);

  const char *str = "Sunny";
  mp_encode_str(&enc_stream, str, strlen(str));
  mp_encode_bool(&enc_stream, true);

  printf("[STREAM ENCODE] Wrote array sequentially: [98.6, -10, \"Sunny\", "
         "true]\n");
  printf("[STREAM ENCODE] Successfully serialized to %zu bytes.\n\n",
         out_buffer->size);
  return 0;
}

int decode_stream_example(const char *data, size_t size) {
  printf("--- DECODING VIA STREAMING API ---\n");
  mp_stream_buffer_t dec_buffer;
  mp_stream_t dec_stream;
  mp_stream_init_read(&dec_stream, &dec_buffer, data, size);

  uint32_t array_len;
  mp_decode_stream_array_len(&dec_stream, &array_len);

  if (array_len == 4) {
    double val_double;
    mp_decode_stream_double(&dec_stream, &val_double);

    int64_t val_int64;
    mp_decode_stream_int(&dec_stream, &val_int64);

    uint32_t val_char_len;
    mp_decode_stream_str_len(&dec_stream, &val_char_len);

    char val_char[32] = {0};
    dec_stream.read(&dec_stream, val_char,
                    val_char_len); // Manually fetch string body

    bool val_bool;
    mp_decode_stream_bool(&dec_stream, &val_bool);

    printf("[STREAM DECODE] Index 0 (Double): %.1f\n", val_double);
    printf("[STREAM DECODE] Index 1 (Int)   : %lld\n", (long long)val_int64);
    printf("[STREAM DECODE] Index 2 (String): %s\n", val_char);
    printf("[STREAM DECODE] Index 3 (Bool)  : %s\n", val_bool ? "true" : "false");
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
  mp_stream_buffer_t builder_buffer;
  if (encode_builder_example(&zone, &builder_buffer) == 0) {
    decode_builder_example(&zone, builder_buffer.data, builder_buffer.size);
    mp_memory_stream_destroy(&builder_buffer);
  }

  // 2. Stream Test
  mp_stream_buffer_t stream_buffer;
  if (encode_stream_example(&stream_buffer) == 0) {
    decode_stream_example(stream_buffer.data, stream_buffer.size);
    mp_memory_stream_destroy(&stream_buffer);
  }

  mp_zone_destroy(&zone);
  return 0;
}
