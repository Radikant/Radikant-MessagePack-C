#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <radikant-messagepack-c.h>

int encode_large_payload(mp_zone_t *zone, mp_stream_buffer_t *out_buffer) {
  // Let's create an employee record map (simulating a large payload)
  mp_object_t map;
  mp_build_map(zone, &map, 4);

  mp_map_set_str(&map, 0, "employee", "Charlie");
  mp_map_set_double(&map, 1, "salary", 111400);
  mp_map_set_bool(&map, 2, "is_manager", true);
  mp_map_set_int(&map, 3, "age", 37);

  printf("[ENCODE] Serializing employee record map: {\"employee\": \"Charlie\", "
         "\"salary\": 111400.0, \"is_manager\": true, \"age\": 37}\n");

  mp_stream_t enc_stream;
  mp_stream_init_write(&enc_stream, out_buffer, true, NULL, 0);

  if (mp_encode_object(&enc_stream, &map) != MP_OK) return 1;

  printf("[ENCODE] Successfully serialized to %zu bytes of binary data.\n\n", out_buffer->size);
  return 0;
}

int query_age(const char* data, size_t size) {
  printf("[QUERY] Searching stream for 'age' without allocating an AST...\n");

  mp_stream_buffer_t dec_buffer;
  mp_stream_t stream;
  mp_stream_init_read(&stream, &dec_buffer, data, size);

  // 1. Initialize decoder WITHOUT a memory zone (Zero Allocation Mode)
  mp_decoder_t decoder;
  mp_decoder_init(&decoder, &stream, NULL);

  // 2. Query the map for the key "age"
  mp_error_t err = mp_query_map_key_str(&decoder, "age");
  if (err == MP_OK) {
      
      // 3. We found it! The decoder is perfectly positioned at the value.
      // We can use the SAX streaming API to read the value instantly.
      int64_t age = 0;
      if (mp_decode_stream_int(decoder.stream, &age) == MP_OK) {
          printf("[SUCCESS] Found Age: %lld\n", (long long)age);
          return 0;
      } else {
          printf("[ERROR] 'age' value was not an integer!\n");
          return 1;
      }
      
  } else if (err == MP_ERROR_QUERY_NOT_FOUND) {
      printf("[ERROR] The key 'age' was not found in the map.\n");
      return 1;
  } else {
      printf("[ERROR] An error occurred while parsing: %d\n", err);
      return 1;
  }
}

int main() {
  printf("======================================\n");
  printf(" Radikant MessagePack-C | Query API\n");
  printf("======================================\n\n");

  mp_zone_t zone;
  mp_zone_init(&zone, 4096);
  mp_stream_buffer_t buffer;

  // Step 1: Create our test data
  if (encode_large_payload(&zone, &buffer) != 0) {
    mp_zone_destroy(&zone);
    return 1;
  }

  // Step 2: Use the Query API to find a specific key instantly
  query_age(buffer.data, buffer.size);

  printf("\n======================================\n");

  // Cleanup
  mp_zone_destroy(&zone);
  mp_memory_stream_destroy(&buffer);

  return 0;
}
