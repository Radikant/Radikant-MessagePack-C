#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <radikant-messagepack-c.h>

// Helper to simulate receiving a massive deep payload
int encode_massive_payload(mp_zone_t *zone, mp_stream_buffer_t *out_buffer) {
  mp_object_t root_map;
  mp_build_map(zone, &root_map, 2);

  // Payload: {"status": "success", "data": {"users": [...]}}
  mp_map_set_str(&root_map, 0, "status", "success");

  mp_object_t data_map;
  mp_build_map(zone, &data_map, 1);
  
  mp_object_t users_arr;
  mp_build_array(zone, &users_arr, 2);
  
  // User 0
  mp_object_t user0;
  mp_build_map(zone, &user0, 5); // 5 fields per user
  mp_map_set_int(&user0, 0, "id", 101);
  mp_map_set_str(&user0, 1, "name", "Alice");
  mp_map_set_str(&user0, 2, "secret_token", "xxx");
  mp_map_set_str(&user0, 3, "role", "admin");
  mp_map_set_int(&user0, 4, "salary", 95000);
  mp_array_set(&users_arr, 0, user0);
  
  // User 1
  mp_object_t user1;
  mp_build_map(zone, &user1, 5);
  mp_map_set_int(&user1, 0, "id", 102);
  mp_map_set_str(&user1, 1, "name", "Charlie");
  mp_map_set_str(&user1, 2, "secret_token", "yyy");
  mp_map_set_str(&user1, 3, "role", "editor");
  mp_map_set_int(&user1, 4, "salary", 80000);
  mp_array_set(&users_arr, 1, user1);
  
  // Link it all up
  mp_object_t users_key;
  mp_build_cstr(&users_key, "users");
  mp_map_set(&data_map, 0, users_key, users_arr);
  
  mp_object_t data_key;
  mp_build_cstr(&data_key, "data");
  mp_map_set(&root_map, 1, data_key, data_map);

  mp_stream_t enc_stream;
  mp_stream_init_write(&enc_stream, out_buffer, true, NULL, 0);
  return (mp_encode_object(&enc_stream, &root_map) == MP_OK) ? 0 : 1;
}

int main() {
  printf("==================================================\n");
  printf(" Radikant MessagePack-C | Advanced Query API \n");
  printf("==================================================\n\n");

  mp_zone_t zone;
  mp_zone_init(&zone, 4096);
  mp_stream_buffer_t buffer;

  // 1. Build the payload
  if (encode_massive_payload(&zone, &buffer) != 0) {
    printf("Failed to encode payload.\n");
    return 1;
  }
  printf("[ENCODE] Generated huge nested payload (Simulated).\n\n");

  // 2. Setup the stream decoder
  mp_stream_buffer_t read_buf;
  mp_stream_t stream;
  mp_stream_init_read(&stream, &read_buf, buffer.data, buffer.size);
  mp_decoder_t decoder;
  mp_decoder_init(&decoder, &stream, NULL); // No zone needed yet!

  // ---------------------------------------------------------
  // CONCEPT 1: Query Paths (Deep Plucking)
  // Let's instantly drill down to: data -> users -> 1
  // ---------------------------------------------------------
  printf("[QUERY] Executing Path: 'data' -> 'users' -> [1]\n");
  mp_query_t q;
  mp_query_init(&q);
  mp_query_add_path_str(&q, "data");
  mp_query_add_path_str(&q, "users");
  mp_query_add_path_idx(&q, 1);
  
  mp_error_t err = mp_query_execute(&decoder, &q);
  if (err != MP_OK) {
      printf("[ERROR] Failed to execute query path: %d\n", err);
      return 1;
  }
  printf("[SUCCESS] Decoder successfully positioned at 'users[1]' instantly!\n\n");

  // ---------------------------------------------------------
  // CONCEPT 2: Field Extraction (Shallow Flattening)
  // The decoder is now at 'users[1]'. It has 5 fields.
  // We only care about "id" and "role". Let's extract ONLY those.
  // ---------------------------------------------------------
  printf("[EXTRACT] Extracting only fields: 'id', 'role' into AST...\n");
  const char* fields[] = {"id", "role"};
  mp_object_t user_ast;
  
  // We attach our zone here because extraction builds an AST
  err = mp_query_extract_fields(&decoder, &zone, fields, 2, &user_ast);
  
  if (err == MP_OK) {
      printf("[SUCCESS] Extracted Map Size: %u fields (ignored the other 3!)\n", user_ast.via.map.size);
      
      // Print the extracted fields
      for (uint32_t i = 0; i < user_ast.via.map.size; i++) {
          mp_object_t* k = &user_ast.via.map.ptr[i].key;
          mp_object_t* v = &user_ast.via.map.ptr[i].val;
          
          if (v->type == MP_TYPE_POSITIVE_INTEGER) {
              printf("  -> %.*s : %llu\n", (int)k->via.str.size, k->via.str.ptr, (unsigned long long)v->via.u64);
          } else if (v->type == MP_TYPE_STR) {
              printf("  -> %.*s : %.*s\n", (int)k->via.str.size, k->via.str.ptr, (int)v->via.str.size, v->via.str.ptr);
          }
      }
  } else {
      printf("[ERROR] Failed to extract fields: %d\n", err);
  }

  printf("\n==================================================\n");

  mp_zone_destroy(&zone);
  mp_memory_stream_destroy(&buffer);
  return 0;
}
