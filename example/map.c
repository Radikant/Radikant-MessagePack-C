#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <radikant-messagepack-c.h>

int encode_example(mp_zone_t *zone, mp_memory_stream_ctx_t *out_ctx) {
  // ---------------------------------------------------------
  // 2. Build a MessagePack Object (AST)
  // ---------------------------------------------------------
  // Let's create an employee record map
  mp_object_t map;
  mp_build_map(zone, &map, 4);

  mp_map_set_str(&map, 0, "employee", "Charlie");
  mp_map_set_int(&map, 1, "age", 29);
  mp_map_set_double(&map, 2, "salary", 111400);
  mp_map_set_bool(&map, 3, "is_manager", true);

  printf("[ENCODE] Building AST map: {\"employee\": \"Charlie\", \"age\": 29, "
         "\"salary\": 75000.50, \"is_manager\": true}\n");

  // ---------------------------------------------------------
  // 3. Encode the Object to a Memory Stream
  // ---------------------------------------------------------
  mp_stream_t enc_stream;

  // Using a dynamic stream automatically handles memory reallocation!
  mp_stream_init_write(&enc_stream, out_ctx, true, NULL, 0);

  // Perform the encoding
  mp_error_t err = mp_encode_object(&enc_stream, &map);
  if (err != MP_OK) {
    printf("Failed to encode object!\n");
    return 1;
  }

  printf("[ENCODE] Successfully serialized to %zu bytes of binary data.\n\n",
         out_ctx->size);

  return 0;
}

int decode_example(mp_zone_t *zone, mp_memory_stream_ctx_t *in_ctx) {
  // ---------------------------------------------------------
  // 4. Decode the Binary Data back to an AST
  // ---------------------------------------------------------
  mp_memory_stream_ctx_t dec_ctx;
  mp_stream_t dec_stream;

  // Read from the dynamically allocated buffer
  mp_stream_init_read(&dec_stream, &dec_ctx, in_ctx->data, in_ctx->size);

  mp_decoder_t decoder;
  mp_decoder_init(&decoder, &dec_stream, zone);

  mp_object_t decoded_ast;
  mp_error_t err = mp_decode(&decoder, &decoded_ast);
  if (err != MP_OK) {
    printf("Failed to decode object! Error: %d\n", err);
    return 1;
  }

  printf("[DECODE] Successfully parsed binary data back into an AST.\n");

  // ---------------------------------------------------------
  // 5. Verify the Decoded Data
  // ---------------------------------------------------------
  if (decoded_ast.type == MP_TYPE_MAP && decoded_ast.via.map.size == 4) {
    mp_object_t *val1 = &decoded_ast.via.map.ptr[0].val; // employee
    mp_object_t *val2 = &decoded_ast.via.map.ptr[1].val; // age
    mp_object_t *val3 = &decoded_ast.via.map.ptr[2].val; // salary
    mp_object_t *val4 = &decoded_ast.via.map.ptr[3].val; // is_manager

    printf("[VERIFY] Employee  : %.*s\n", (int)val1->via.str.size,
           val1->via.str.ptr);
    printf("[VERIFY] Age       : %llu\n", (unsigned long long)val2->via.u64);
    printf("[VERIFY] Salary    : %.2f\n", val3->via.f64);
    printf("[VERIFY] Is Manager: %s\n", val4->via.boolean ? "Yes" : "No");
  }

  return 0;
}

int main() {
  printf("======================================\n");
  printf(" Radikant MessagePack-C Example\n");
  printf("======================================\n\n");

  // ---------------------------------------------------------
  // 1. Initialize an Allocator (Zone)
  // ---------------------------------------------------------
  mp_zone_t zone;
  mp_zone_init(&zone, 4096);

  mp_memory_stream_ctx_t enc_ctx;

  if (encode_example(&zone, &enc_ctx) != 0) {
    mp_zone_destroy(&zone);
    return 1;
  }

  if (decode_example(&zone, &enc_ctx) != 0) {
    mp_zone_destroy(&zone);
    mp_memory_stream_destroy(&enc_ctx);
    return 1;
  }

  printf("\n======================================\n");

  // ---------------------------------------------------------
  // 6. Cleanup
  // ---------------------------------------------------------
  mp_zone_destroy(&zone);
  mp_memory_stream_destroy(&enc_ctx);

  return 0;
}