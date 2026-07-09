#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <radikant-messagepack-c.h>

int main() {
  printf("======================================\n");
  printf(" Radikant MessagePack-C | Filter API \n");
  printf("======================================\n\n");

  mp_zone_t zone;
  mp_zone_init(&zone, 4096);

  // 1. Build an array of 5 Employees
  mp_object_t arr;
  mp_build_array(&zone, &arr, 5);

  const char *names[] = {"Alice", "Charlie", "Bob", "Charlie", "Eve"};
  int ages[] = {42, 37, 38, 37, 26};
  const char *roles[] = {"CEO", "Engineer", "CTO", "Manager", "QA"};

  for (int i = 0; i < 5; i++) {
    mp_object_t emp;
    mp_build_map(&zone, &emp, 3);
    mp_map_set_str(&emp, 0, "name", names[i]);
    mp_map_set_int(&emp, 1, "age", ages[i]);
    mp_map_set_str(&emp, 2, "role", roles[i]);
    mp_array_set(&arr, i, emp);
  }

  mp_print_object(&arr);

  printf("[ENCODE] Serializing massive array of %d employees into a binary "
         "stream...\n",
         arr.via.array.size);

  mp_stream_buffer_t buffer;
  mp_stream_t stream;
  mp_stream_init_write(&stream, &buffer, true, NULL, 0);
  mp_encode_object(&stream, &arr);

  printf("[ENCODE] Encoded payload is %zu bytes.\n\n", buffer.size);

  // 2. We want to find the Charlie who is exactly 37 years old.
  mp_stream_t read_stream;
  mp_stream_init_read(&read_stream, &buffer, buffer.data, buffer.size);

  mp_decoder_t decoder;
  mp_decoder_init(&decoder, &read_stream, NULL);

  mp_filter_t filter;
  mp_filter_init(&filter);
  mp_filter_add_str(&filter, "name", "Charlie");
  mp_filter_add_int(&filter, "age", 37);

  printf("[FILTER] Instantly scanning binary stream for 'name' == 'Charlie' "
         "AND 'age' == 37...\n");

  mp_object_t matches;
  mp_error_t err = mp_query_filter_array(&decoder, &zone, &filter, &matches);

  if (err == MP_OK) {
    printf("[SUCCESS] Filter executed flawlessly at zero-allocation speed!\n");
    printf("[SUCCESS] Found %u match(es):\n\n", matches.via.array.size);

    // Print the matches beautifully!
    mp_print_object(&matches);
  } else {
    printf("[ERROR] Filter failed with code: %d\n", err);
  }

  printf("\n======================================\n");

  mp_zone_destroy(&zone);
  mp_memory_stream_destroy(&buffer);
  return 0;
}
