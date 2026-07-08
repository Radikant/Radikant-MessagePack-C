#include <stdio.h>
#include <stdlib.h>

#include <radikant-messagepack-c.h>

int encode_example(mp_zone_t *zone, char **out_data, size_t *out_size) {
  // 1. Build a simple AST array using the Builder APIs.
  mp_object_t array;
  mp_build_array(zone, &array, 3);

  mp_object_t item;
  mp_build_int(&item, 42);
  mp_array_set(&array, 0, item);

  mp_build_int(&item, 99);
  mp_array_set(&array, 1, item);

  mp_build_cstr(&item, "Hello");
  mp_array_set(&array, 2, item);

  printf("[ENCODE] Building Array: [42, 99, \"Hello\"]\n");

  // 2. Serialize the AST to a dynamic memory buffer in one simple line!
  // No manual streams or contexts required.
  mp_error_t err = mp_serialize_memory(&array, out_data, out_size);
  if (err != MP_OK) {
    printf("Failed to serialize! Error: %d\n", err);
    return 1;
  }

  printf("[ENCODE] Successfully encoded to %zu bytes.\n\n", *out_size);
  return 0;
}

int decode_example(mp_zone_t *zone, const char *encoded_data,
                   size_t encoded_size) {
  // 3. Parse the binary data back into an AST in one simple line!
  // Memory for the new AST is safely allocated inside the zone.
  mp_object_t parsed_ast;
  mp_error_t err =
      mp_parse_memory(zone, encoded_data, encoded_size, &parsed_ast);
  if (err != MP_OK) {
    printf("Failed to parse! Error: %d\n", err);
    return 1;
  }

  printf("[DECODE] Successfully parsed AST from memory.\n");

  // 4. Verify the results
  if (parsed_ast.type == MP_TYPE_ARRAY && parsed_ast.via.array.size == 3) {
    printf("[VERIFY] Int 1: %lld\n", parsed_ast.via.array.ptr[0].via.u64);
    printf("[VERIFY] Int 2: %lld\n", parsed_ast.via.array.ptr[1].via.u64);
    printf("[VERIFY] String : %.*s\n",
           (int)parsed_ast.via.array.ptr[2].via.str.size,
           parsed_ast.via.array.ptr[2].via.str.ptr);
  }

  return 0;
}

int main() {
  printf("======================================\n");
  printf(" Radikant MessagePack-C Basic Example\n");
  printf("======================================\n\n");

  // We need a Zone to allocate our MessagePack objects.
  mp_zone_t zone;
  mp_zone_init(&zone, 4096);

  char *encoded_data = NULL;
  size_t encoded_size = 0;

  if (encode_example(&zone, &encoded_data, &encoded_size) != 0) {
    mp_zone_destroy(&zone);
    return 1;
  }

  if (decode_example(&zone, encoded_data, encoded_size) != 0) {
    free(encoded_data);
    mp_zone_destroy(&zone);
    return 1;
  }

  // Clean up
  free(encoded_data);     // Free the serialized buffer
  mp_zone_destroy(&zone); // Free all AST nodes simultaneously

  return 0;
}
