#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <radikant-messagepack-c.h>

// ============================================================================
// 1. Encoding (Creating a nested MessagePack object)
// ============================================================================
int example_encode_nested(mp_zone_t *zone, mp_stream_buffer_t *out_buffer) {
  // We want to create the following JSON-like structure:
  // {
  //   "employee": "Charlie",
  //   "age": 36,
  //   "address": {
  //     "city": "Berlin",
  //     "zipcode": 10115
  //   },
  //   "skills": ["C", "MessagePack", "Performance"]
  // }

  // --- A. Create the root Map (4 items) ---
  mp_object_t root_map;
  mp_build_map(zone, &root_map, 4);

  // 1st and 2nd items: Simple key-value pairs
  mp_map_set_str(&root_map, 0, "employee", "Charlie");
  mp_map_set_int(&root_map, 1, "age", 29);

  // --- B. Create the nested "address" Map (2 items) ---
  mp_object_t address_map;
  mp_build_map(zone, &address_map, 2);
  mp_map_set_str(&address_map, 0, "city", "Berlin");
  mp_map_set_int(&address_map, 1, "zipcode", 10115);

  // Attach "address" map to the root map
  mp_object_t addr_key;
  mp_build_cstr(&addr_key, "address");
  mp_map_set(&root_map, 2, addr_key, address_map);

  // --- C. Create the nested "skills" Array (3 items) ---
  mp_object_t skills_array;
  mp_build_array(zone, &skills_array, 3);

  mp_object_t skill;
  mp_build_cstr(&skill, "C");
  mp_array_set(&skills_array, 0, skill);

  mp_build_cstr(&skill, "MessagePack");
  mp_array_set(&skills_array, 1, skill);

  mp_build_cstr(&skill, "Performance");
  mp_array_set(&skills_array, 2, skill);

  // Attach "skills" array to the root map
  mp_object_t skills_key;
  mp_build_cstr(&skills_key, "skills");
  mp_map_set(&root_map, 3, skills_key, skills_array);

  // --- D. Encode the complete nested structure ---
  printf("[ENCODE] Encoding nested employee record...\n");

  mp_stream_t enc_stream;
  mp_stream_init_write(&enc_stream, out_buffer, true, NULL, 0);

  mp_error_t err = mp_encode_object(&enc_stream, &root_map);
  if (err != MP_OK) {
    printf("[ENCODE] Failed to encode object! Error: %d\n", err);
    return 1;
  }

  printf("[ENCODE] Successfully serialized to %zu bytes of binary data.\n\n",
         out_buffer->size);

  return 0;
}

// ============================================================================
// 2. Decoding (Reading a nested MessagePack object)
// ============================================================================
int example_decode_nested(mp_zone_t *zone, mp_stream_buffer_t *in_buffer) {
  // --- A. Read the binary data back into an AST ---
  mp_stream_buffer_t dec_buffer;
  mp_stream_t dec_stream;
  mp_stream_init_read(&dec_stream, &dec_buffer, in_buffer->data, in_buffer->size);

  mp_decoder_t decoder;
  mp_decoder_init(&decoder, &dec_stream, zone);

  mp_object_t root_ast;
  mp_error_t err = mp_decode(&decoder, &root_ast);
  if (err != MP_OK) {
    printf("[DECODE] Failed to decode object! Error: %d\n", err);
    return 1;
  }
  printf("[DECODE] Successfully parsed binary data back into an AST.\n\n");

  // --- B. Verify and print the data ---
  // In a real app, you would check types carefully before accessing via.ptr
  if (root_ast.type == MP_TYPE_MAP && root_ast.via.map.size == 4) {
    mp_object_t *employee = &root_ast.via.map.ptr[0].val;
    mp_object_t *age = &root_ast.via.map.ptr[1].val;
    mp_object_t *address = &root_ast.via.map.ptr[2].val;
    mp_object_t *skills = &root_ast.via.map.ptr[3].val;

    printf("[VERIFY] Employee: %.*s\n", (int)employee->via.str.size,
           employee->via.str.ptr);
    printf("[VERIFY] Age     : %llu\n", (unsigned long long)age->via.u64);

    // Read into the nested Address map
    if (address->type == MP_TYPE_MAP) {
      mp_object_t *city = &address->via.map.ptr[0].val;
      mp_object_t *zipcode = &address->via.map.ptr[1].val;
      printf("[VERIFY] Address : %.*s %llu\n", (int)city->via.str.size,
             city->via.str.ptr, (unsigned long long)zipcode->via.u64);
    }

    // Read into the nested Skills array
    if (skills->type == MP_TYPE_ARRAY) {
      printf("[VERIFY] Skills  : ");
      for (uint32_t i = 0; i < skills->via.array.size; i++) {
        mp_object_t *skill_item = &skills->via.array.ptr[i];
        printf("%.*s ", (int)skill_item->via.str.size, skill_item->via.str.ptr);
      }
      printf("\n");
    }
  }

  return 0;
}

// ============================================================================
// 3. Main Runner
// ============================================================================
int main() {
  printf("======================================\n");
  printf(" Radikant Nested Data Example\n");
  printf("======================================\n\n");

  // 1. Initialize our memory zone (freed at the end)
  mp_zone_t zone;
  mp_zone_init(&zone, 4096);

  mp_stream_buffer_t enc_buffer;

  // 2. Encode
  if (example_encode_nested(&zone, &enc_buffer) != 0) {
    mp_zone_destroy(&zone);
    return 1;
  }

  // 3. Decode
  if (example_decode_nested(&zone, &enc_buffer) != 0) {
    mp_zone_destroy(&zone);
    mp_memory_stream_destroy(&enc_buffer);
    return 1;
  }

  printf("\n======================================\n");

  // 4. Cleanup
  mp_zone_destroy(&zone);
  mp_memory_stream_destroy(&enc_buffer);

  return 0;
}
