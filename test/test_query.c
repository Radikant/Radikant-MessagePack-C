
// std
#include <string.h>

// Radikant
#include <radikant-messagepack-c.h>
#include <radikant-probe-c.h>

bool test_query_map_key_str(test_result_t *test) {
  mp_zone_t zone;
  mp_zone_init(&zone, 4096);

  // Build test map: {"foo": 10, "bar": 20, "baz": 30}
  mp_object_t map;
  mp_build_map(&zone, &map, 3);
  mp_map_set_int(&map, 0, "foo", 10);
  mp_map_set_int(&map, 1, "bar", 20);
  mp_map_set_int(&map, 2, "baz", 30);

  mp_stream_buffer_t buffer;
  mp_stream_t enc_stream;
  mp_stream_init_write(&enc_stream, &buffer, true, NULL, 0);
  mp_encode_object(&enc_stream, &map);

  // Test 1: Query for an existing key ("bar")
  mp_stream_buffer_t read_buf;
  mp_stream_t stream;
  mp_stream_init_read(&stream, &read_buf, buffer.data, buffer.size);
  mp_decoder_t decoder;
  mp_decoder_init(&decoder, &stream, NULL);

  mp_error_t err = mp_query_map_key_str(&decoder, "bar");
  if (err != MP_OK)
    append_error(test, "Failed to query existing key 'bar'", err);

  int64_t val = 0;
  if (mp_decode_stream_int(&stream, &val) != MP_OK || val != 20) {
    append_error(test, "Incorrect value retrieved for 'bar'", val);
  }

  // Test 2: Query for a non-existent key ("qux")
  mp_stream_init_read(&stream, &read_buf, buffer.data, buffer.size);
  mp_decoder_init(&decoder, &stream, NULL);

  err = mp_query_map_key_str(&decoder, "qux");
  if (err != MP_ERROR_QUERY_NOT_FOUND)
    append_error(test, "Expected NOT_FOUND for missing key", err);

  mp_zone_destroy(&zone);
  mp_memory_stream_destroy(&buffer);
  return test_end(test);
}

bool test_query_array_index(test_result_t *test) {
  mp_zone_t zone;
  mp_zone_init(&zone, 4096);

  // Build test array: [100, 200, 300, 400]
  mp_object_t arr;
  mp_build_array(&zone, &arr, 4);
  mp_array_set_int(&arr, 0, 100);
  mp_array_set_int(&arr, 1, 200);
  mp_array_set_int(&arr, 2, 300);
  mp_array_set_int(&arr, 3, 400);

  mp_stream_buffer_t buffer;
  mp_stream_t enc_stream;
  mp_stream_init_write(&enc_stream, &buffer, true, NULL, 0);
  mp_encode_object(&enc_stream, &arr);

  // Test 1: Query for valid index (2)
  mp_stream_buffer_t read_buf;
  mp_stream_t stream;
  mp_stream_init_read(&stream, &read_buf, buffer.data, buffer.size);
  mp_decoder_t decoder;
  mp_decoder_init(&decoder, &stream, NULL);

  mp_error_t err = mp_query_array_index(&decoder, 2);
  if (err != MP_OK)
    append_error(test, "Failed to query valid index 2", err);

  int64_t val = 0;
  if (mp_decode_stream_int(&stream, &val) != MP_OK || val != 300) {
    append_error(test, "Incorrect value retrieved for index 2", val);
  }

  // Test 2: Query for out-of-bounds index (5)
  mp_stream_init_read(&stream, &read_buf, buffer.data, buffer.size);
  mp_decoder_init(&decoder, &stream, NULL);

  err = mp_query_array_index(&decoder, 5);
  if (err != MP_ERROR_QUERY_NOT_FOUND)
    append_error(test, "Expected NOT_FOUND for out-of-bounds index", err);

  mp_zone_destroy(&zone);
  mp_memory_stream_destroy(&buffer);
  return test_end(test);
}

bool test_query_bad_args(test_result_t *test) {
  mp_stream_t stream;
  mp_decoder_t decoder;
  mp_decoder_init(&decoder, &stream, NULL);

  if (mp_query_map_key_str(NULL, "foo") != MP_ERROR_BAD_ARG)
    append_error(test, "Expected BAD_ARG for NULL decoder", 0);
  if (mp_query_map_key_str(&decoder, NULL) != MP_ERROR_BAD_ARG)
    append_error(test, "Expected BAD_ARG for NULL key", 0);
  if (mp_query_array_index(NULL, 0) != MP_ERROR_BAD_ARG)
    append_error(test, "Expected BAD_ARG for NULL decoder", 0);

  return test_end(test);
}

bool test_query_type_mismatch(test_result_t *test) {
  mp_zone_t zone;
  mp_zone_init(&zone, 4096);

  // Create an Array but try to query it as a Map
  mp_object_t arr;
  mp_build_array(&zone, &arr, 1);
  mp_array_set_int(&arr, 0, 42);

  mp_stream_buffer_t buffer;
  mp_stream_t enc_stream;
  mp_stream_init_write(&enc_stream, &buffer, true, NULL, 0);
  mp_encode_object(&enc_stream, &arr);

  mp_stream_buffer_t read_buf;
  mp_stream_t stream;
  mp_stream_init_read(&stream, &read_buf, buffer.data, buffer.size);
  mp_decoder_t decoder;
  mp_decoder_init(&decoder, &stream, NULL);

  mp_error_t err = mp_query_map_key_str(&decoder, "foo");
  if (err != MP_ERROR_QUERY_TYPE_MISMATCH) {
    append_error(test, "Expected type mismatch when querying array as map",
                 err);
  }

  mp_zone_destroy(&zone);
  mp_memory_stream_destroy(&buffer);
  return test_end(test);
}

bool test_query_complex_map(test_result_t *test) {
  mp_zone_t zone;
  mp_zone_init(&zone, 4096);

  // Create a map with mixed key types to stress-test the skipping logic!
  mp_object_t map;
  mp_build_map(&zone, &map, 4);

  // 1. Integer key
  mp_object_t key1 = {.type = MP_TYPE_POSITIVE_INTEGER, .via.u64 = 100};
  mp_object_t val1 = {.type = MP_TYPE_STR,
                      .via.str = {.ptr = "val1", .size = 4}};
  map.via.map.ptr[0].key = key1;
  map.via.map.ptr[0].val = val1;

  // 2. Boolean key
  mp_object_t key2 = {.type = MP_TYPE_BOOLEAN, .via.boolean = true};
  mp_object_t val2 = {.type = MP_TYPE_STR,
                      .via.str = {.ptr = "val2", .size = 4}};
  map.via.map.ptr[1].key = key2;
  map.via.map.ptr[1].val = val2;

  // 3. String key with similar length (to test false positives)
  mp_map_set_str(&map, 2, "far", "val3"); // we will search for "foo"

  // 4. The actual target key
  mp_map_set_int(&map, 3, "foo", 999);

  mp_stream_buffer_t buffer;
  mp_stream_t enc_stream;
  mp_stream_init_write(&enc_stream, &buffer, true, NULL, 0);
  mp_encode_object(&enc_stream, &map);

  mp_stream_buffer_t read_buf;
  mp_stream_t stream;
  mp_stream_init_read(&stream, &read_buf, buffer.data, buffer.size);
  mp_decoder_t decoder;
  mp_decoder_init(&decoder, &stream, NULL);

  mp_error_t err = mp_query_map_key_str(&decoder, "foo");
  if (err != MP_OK)
    append_error(test, "Failed to find 'foo' in complex map", err);

  int64_t val = 0;
  if (mp_decode_stream_int(&stream, &val) != MP_OK || val != 999) {
    append_error(test, "Incorrect value retrieved from complex map", val);
  }

  mp_zone_destroy(&zone);
  mp_memory_stream_destroy(&buffer);
  return test_end(test);
}

bool test_query_execute_path(test_result_t *test) {
    mp_zone_t zone;
    mp_zone_init(&zone, 4096);
    
    // Build nested payload: {"users": [{"name": "A", "age": 20}, {"name": "B", "age": 25}]}
    mp_object_t map;
    mp_build_map(&zone, &map, 1);
    
    mp_object_t arr;
    mp_build_array(&zone, &arr, 2);
    
    mp_object_t user1, user2;
    mp_build_map(&zone, &user1, 2);
    mp_map_set_str(&user1, 0, "name", "A");
    mp_map_set_int(&user1, 1, "age", 20);
    
    mp_build_map(&zone, &user2, 2);
    mp_map_set_str(&user2, 0, "name", "B");
    mp_map_set_int(&user2, 1, "age", 25);
    
    mp_array_set(&arr, 0, user1);
    mp_array_set(&arr, 1, user2);
    
    mp_object_t users_key;
    mp_build_cstr(&users_key, "users");
    mp_map_set(&map, 0, users_key, arr);
    
    mp_stream_buffer_t buffer;
    mp_stream_t enc_stream;
    mp_stream_init_write(&enc_stream, &buffer, true, NULL, 0);
    mp_encode_object(&enc_stream, &map);
    
    mp_stream_buffer_t read_buf;
    mp_stream_t stream;
    mp_stream_init_read(&stream, &read_buf, buffer.data, buffer.size);
    mp_decoder_t decoder;
    mp_decoder_init(&decoder, &stream, NULL);
    
    mp_query_t q;
    mp_query_init(&q);
    mp_query_add_path_str(&q, "users");
    mp_query_add_path_idx(&q, 1);
    mp_query_add_path_str(&q, "age");
    
    mp_error_t err = mp_query_execute(&decoder, &q);
    if (err != MP_OK) append_error(test, "Query execute path failed", err);
    
    int64_t val = 0;
    if (mp_decode_stream_int(&stream, &val) != MP_OK || val != 25) {
        append_error(test, "Query path retrieved incorrect value", val);
    }
    
    mp_zone_destroy(&zone);
    mp_memory_stream_destroy(&buffer);
    return test_end(test);
}

bool test_query_extract_fields(test_result_t *test) {
    mp_zone_t zone;
    mp_zone_init(&zone, 4096);
    
    mp_object_t map;
    mp_build_map(&zone, &map, 5);
    mp_map_set_int(&map, 0, "id", 101);
    mp_map_set_str(&map, 1, "name", "Charlie");
    mp_map_set_str(&map, 2, "ignored1", "x");
    mp_map_set_str(&map, 3, "role", "admin");
    mp_map_set_str(&map, 4, "ignored2", "y");
    
    mp_stream_buffer_t buffer;
    mp_stream_t enc_stream;
    mp_stream_init_write(&enc_stream, &buffer, true, NULL, 0);
    mp_encode_object(&enc_stream, &map);
    
    mp_stream_buffer_t read_buf;
    mp_stream_t stream;
    mp_stream_init_read(&stream, &read_buf, buffer.data, buffer.size);
    mp_decoder_t decoder;
    mp_decoder_init(&decoder, &stream, NULL);
    
    const char* fields[] = {"id", "role"};
    mp_object_t out;
    
    mp_zone_t ex_zone;
    mp_zone_init(&ex_zone, 4096);
    
    mp_error_t err = mp_query_extract_fields(&decoder, &ex_zone, fields, 2, &out);
    if (err != MP_OK) append_error(test, "Extract fields failed", err);
    
    if (out.type != MP_TYPE_MAP || out.via.map.size != 2) {
        append_error(test, "Extracted map size mismatch", out.via.map.size);
    }
    
    // Check if the keys actually match what we asked for
    bool found_id = false;
    bool found_role = false;
    
    for (uint32_t i = 0; i < out.via.map.size; i++) {
        mp_object_t* k = &out.via.map.ptr[i].key;
        mp_object_t* v = &out.via.map.ptr[i].val;
        if (strncmp(k->via.str.ptr, "id", k->via.str.size) == 0 && v->via.u64 == 101) found_id = true;
        if (strncmp(k->via.str.ptr, "role", k->via.str.size) == 0) found_role = true;
    }
    
    if (!found_id) append_error(test, "Did not extract 'id'", 0);
    if (!found_role) append_error(test, "Did not extract 'role'", 0);
    
    mp_zone_destroy(&ex_zone);
    mp_zone_destroy(&zone);
    mp_memory_stream_destroy(&buffer);
    return test_end(test);
}

bool test_query_filter_array(test_result_t *test) {
    mp_zone_t zone;
    mp_zone_init(&zone, 4096);

    // Build an array of 3 employees
    mp_object_t arr;
    mp_build_array(&zone, &arr, 3);

    mp_object_t e1, e2, e3;
    mp_build_map(&zone, &e1, 2);
    mp_map_set_str(&e1, 0, "name", "Alice");
    mp_map_set_int(&e1, 1, "age", 42);

    mp_build_map(&zone, &e2, 2);
    mp_map_set_str(&e2, 0, "name", "Charlie");
    mp_map_set_int(&e2, 1, "age", 37);

    mp_build_map(&zone, &e3, 2);
    mp_map_set_str(&e3, 0, "name", "Charlie");
    mp_map_set_int(&e3, 1, "age", 25);

    mp_array_set(&arr, 0, e1);
    mp_array_set(&arr, 1, e2);
    mp_array_set(&arr, 2, e3);

    mp_stream_buffer_t buffer;
    mp_stream_t stream;
    mp_stream_init_write(&stream, &buffer, true, NULL, 0);
    mp_encode_object(&stream, &arr);

    mp_stream_t read_stream;
    mp_stream_init_read(&read_stream, &buffer, buffer.data, buffer.size);
    mp_decoder_t decoder;
    mp_decoder_init(&decoder, &read_stream, NULL);

    mp_filter_t filter;
    mp_filter_init(&filter);
    mp_filter_add_str(&filter, "name", "Charlie");
    mp_filter_add_int(&filter, "age", 37);

    mp_object_t results;
    mp_error_t err = mp_query_filter_array(&decoder, &zone, &filter, &results);
    
    if (err != MP_OK) append_error(test, "Filter executed failed", err);
    if (results.type != MP_TYPE_ARRAY) append_error(test, "Result not an array", 0);
    if (results.via.array.size != 1) append_error(test, "Filter count mismatch", results.via.array.size);

    mp_zone_destroy(&zone);
    mp_memory_stream_destroy(&buffer);
    return test_end(test);
}

bool test_query_long_string_keys(test_result_t *test) {
    mp_zone_t zone;
    mp_zone_init(&zone, 4096);

    char long_key[300];
    memset(long_key, 'A', 299);
    long_key[299] = '\0';

    mp_object_t map;
    mp_build_map(&zone, &map, 1);
    
    mp_object_t key_obj;
    key_obj.type = MP_TYPE_STR;
    key_obj.via.str.ptr = long_key;
    key_obj.via.str.size = 299;
    
    mp_object_t val_obj;
    val_obj.type = MP_TYPE_POSITIVE_INTEGER;
    val_obj.via.u64 = 42;
    
    map.via.map.ptr[0].key = key_obj;
    map.via.map.ptr[0].val = val_obj;

    mp_stream_buffer_t buffer;
    mp_stream_t stream;
    mp_stream_init_write(&stream, &buffer, true, NULL, 0);
    mp_encode_object(&stream, &map);

    // Test map key string with large key
    mp_stream_t read_stream;
    mp_stream_init_read(&read_stream, &buffer, buffer.data, buffer.size);
    mp_decoder_t decoder;
    mp_decoder_init(&decoder, &read_stream, NULL);
    
    mp_error_t err = mp_query_map_key_str(&decoder, long_key);
    if (err != MP_OK) {
        printf("[DEBUG] mp_query_map_key_str failed with: %d\n", err);
        append_error(test, "Failed to find long string key", err);
    }

    // Test extract fields with large key
    mp_stream_init_read(&read_stream, &buffer, buffer.data, buffer.size);
    mp_decoder_init(&decoder, &read_stream, NULL);
    
    mp_object_t out_map;
    const char *fields[] = {long_key};
    err = mp_query_extract_fields(&decoder, &zone, fields, 1, &out_map);
    if (err != MP_OK) {
        printf("[DEBUG] mp_query_extract_fields failed with: %d\n", err);
        append_error(test, "Failed to extract long string key field", err);
    }
    if (out_map.via.map.size != 1) append_error(test, "Extracted map size mismatch", out_map.via.map.size);

    mp_zone_destroy(&zone);
    mp_memory_stream_destroy(&buffer);
    return test_end(test);
}

bool test_query_filter_bool(test_result_t *test) {
    mp_zone_t zone;
    mp_zone_init(&zone, 4096);

    mp_object_t arr;
    mp_build_array(&zone, &arr, 3);

    mp_object_t e1, e2, e3;
    mp_build_map(&zone, &e1, 1);
    mp_map_set_bool(&e1, 0, "is_admin", true);

    mp_build_map(&zone, &e2, 1);
    mp_map_set_bool(&e2, 0, "is_admin", false);

    mp_build_map(&zone, &e3, 1);
    mp_map_set_bool(&e3, 0, "is_admin", true);

    mp_array_set(&arr, 0, e1);
    mp_array_set(&arr, 1, e2);
    mp_array_set(&arr, 2, e3);

    mp_stream_buffer_t buffer;
    mp_stream_t stream;
    mp_stream_init_write(&stream, &buffer, true, NULL, 0);
    mp_encode_object(&stream, &arr);

    mp_stream_t read_stream;
    mp_stream_init_read(&read_stream, &buffer, buffer.data, buffer.size);
    mp_decoder_t decoder;
    mp_decoder_init(&decoder, &read_stream, NULL);

    mp_filter_t filter;
    mp_filter_init(&filter);
    mp_filter_add_bool(&filter, "is_admin", true);

    mp_object_t results;
    mp_error_t err = mp_query_filter_array(&decoder, &zone, &filter, &results);
    
    if (err != MP_OK) append_error(test, "Filter executed failed", err);
    if (results.via.array.size != 2) append_error(test, "Filter count mismatch (boolean)", results.via.array.size);

    mp_zone_destroy(&zone);
    mp_memory_stream_destroy(&buffer);
    return test_end(test);
}

bool test_query_max_limits(test_result_t *test) {
    mp_query_t q;
    mp_query_init(&q);
    for (int i=0; i<MP_MAX_QUERY_DEPTH; i++) {
        mp_query_add_path_str(&q, "x");
    }
    if (mp_query_add_path_str(&q, "overflow") != MP_ERROR_QUERY_MAX_DEPTH) {
        append_error(test, "Expected MAX_DEPTH when exceeding max query depth", 0);
    }
    
    mp_filter_t f;
    mp_filter_init(&f);
    for (int i=0; i<MP_MAX_FILTER_CONDITIONS; i++) {
        mp_filter_add_str(&f, "x", "y");
    }
    if (mp_filter_add_str(&f, "overflow", "v") != MP_ERROR_BAD_ARG) {
        append_error(test, "Expected BAD_ARG when exceeding max filter conditions", 0);
    }

    return test_end(test);
}

bool test_query_duplicate_keys_extraction(test_result_t *test) {
    mp_zone_t zone;
    mp_zone_init(&zone, 4096);

    mp_object_t map;
    mp_build_map(&zone, &map, 2);
    mp_map_set_int(&map, 0, "dup", 10);
    mp_map_set_int(&map, 1, "dup", 20); // Deliberately invalid JSON/MessagePack, but parser shouldn't overflow

    mp_stream_buffer_t buffer;
    mp_stream_t stream;
    mp_stream_init_write(&stream, &buffer, true, NULL, 0);
    mp_encode_object(&stream, &map);

    mp_stream_t read_stream;
    mp_stream_init_read(&read_stream, &buffer, buffer.data, buffer.size);
    mp_decoder_t decoder;
    mp_decoder_init(&decoder, &read_stream, NULL);
    
    mp_object_t out_map;
    const char *fields[] = {"dup"}; // Ask for 1 field
    mp_error_t err = mp_query_extract_fields(&decoder, &zone, fields, 1, &out_map);
    
    // It should safely fail with MP_ERROR_DECODE_INVALID_FORMAT
    if (err != MP_ERROR_DECODE_INVALID_FORMAT) {
        append_error(test, "Expected INVALID_FORMAT due to duplicate key overflow prevention", err);
    }

    mp_memory_stream_destroy(&buffer);
    return test_end(test);
}

bool test_query_truncated_payload(test_result_t *test) {
    mp_zone_t zone;
    mp_zone_init(&zone, 4096);

    // Create a deeply nested structure with a long string key
    // so we have a nice, juicy payload to truncate.
    mp_object_t map;
    mp_build_map(&zone, &map, 2);
    mp_map_set_int(&map, 0, "simple_key", 100);
    
    mp_object_t inner_arr;
    mp_build_array(&zone, &inner_arr, 2);
    mp_array_set_int(&inner_arr, 0, 10);
    mp_array_set_int(&inner_arr, 1, 20);

    mp_object_t key_obj;
    mp_build_cstr(&key_obj, "nested_array");
    map.via.map.ptr[1].key = key_obj;
    map.via.map.ptr[1].val = inner_arr;

    mp_stream_buffer_t buffer;
    mp_stream_t stream;
    mp_stream_init_write(&stream, &buffer, true, NULL, 0);
    mp_encode_object(&stream, &map);

    // Truncate the buffer at EVERY single possible byte boundary
    for (size_t trunc_size = 1; trunc_size < buffer.size; trunc_size++) {
        mp_stream_t read_stream;
        mp_stream_init_read(&read_stream, &buffer, buffer.data, trunc_size); // Truncated!
        mp_decoder_t decoder;
        mp_decoder_init(&decoder, &read_stream, NULL);
        
        mp_error_t err = mp_query_map_key_str(&decoder, "target");
        
        // As long as it doesn't return MP_OK (because the key isn't there)
        // and doesn't crash, we are good. Usually returns INCOMPLETE or NOT_FOUND.
        if (err == MP_OK) {
            append_error(test, "Truncated buffer miraculously succeeded in finding 'target'", (int64_t)trunc_size);
        }

        // Test Deep path execute on truncated
        mp_stream_init_read(&read_stream, &buffer, buffer.data, trunc_size); // Truncated!
        mp_decoder_init(&decoder, &read_stream, NULL);

        mp_query_t q;
        mp_query_init(&q);
        mp_query_add_path_str(&q, "nested_array");
        mp_query_add_path_idx(&q, 1);
        err = mp_query_execute(&decoder, &q);
        
        if (err == MP_OK) {
            append_error(test, "Truncated buffer miraculously succeeded in executing deep path", (int64_t)trunc_size);
        }
    }

    mp_zone_destroy(&zone);
    mp_memory_stream_destroy(&buffer);
    return test_end(test);
}

bool test_query_extract_missing_fields(test_result_t *test) {
    mp_zone_t zone;
    mp_zone_init(&zone, 4096);
    
    mp_object_t map;
    mp_build_map(&zone, &map, 2);
    mp_map_set_int(&map, 0, "id", 101);
    mp_map_set_str(&map, 1, "name", "Alice");
    
    mp_stream_buffer_t buffer;
    mp_stream_t enc_stream;
    mp_stream_init_write(&enc_stream, &buffer, true, NULL, 0);
    mp_encode_object(&enc_stream, &map);
    
    mp_stream_t read_stream;
    mp_stream_init_read(&read_stream, &buffer, buffer.data, buffer.size);
    mp_decoder_t decoder;
    mp_decoder_init(&decoder, &read_stream, NULL);
    
    const char* fields[] = {"name", "DOES_NOT_EXIST", "ALSO_MISSING"};
    mp_object_t out;
    
    mp_error_t err = mp_query_extract_fields(&decoder, &zone, fields, 3, &out);
    if (err != MP_OK) append_error(test, "Extracting missing fields failed", err);
    if (out.via.map.size != 1) append_error(test, "Expected map size 1", out.via.map.size);
    
    mp_zone_destroy(&zone);
    mp_memory_stream_destroy(&buffer);
    return test_end(test);
}

bool test_query_filter_mixed_array(test_result_t *test) {
    mp_zone_t zone;
    mp_zone_init(&zone, 4096);

    mp_object_t arr;
    mp_build_array(&zone, &arr, 4);

    mp_object_t num1, num2;
    num1.type = MP_TYPE_POSITIVE_INTEGER; num1.via.u64 = 123;
    num2.type = MP_TYPE_POSITIVE_INTEGER; num2.via.u64 = 456;

    mp_object_t map1, map2;
    mp_build_map(&zone, &map1, 1);
    mp_map_set_str(&map1, 0, "role", "admin");
    mp_build_map(&zone, &map2, 1);
    mp_map_set_str(&map2, 0, "role", "user");

    mp_array_set(&arr, 0, num1);
    mp_array_set(&arr, 1, map1);
    mp_array_set(&arr, 2, num2);
    mp_array_set(&arr, 3, map2);

    mp_stream_buffer_t buffer;
    mp_stream_t stream;
    mp_stream_init_write(&stream, &buffer, true, NULL, 0);
    mp_encode_object(&stream, &arr);

    mp_stream_t read_stream;
    mp_stream_init_read(&read_stream, &buffer, buffer.data, buffer.size);
    mp_decoder_t decoder;
    mp_decoder_init(&decoder, &read_stream, NULL);

    mp_filter_t filter;
    mp_filter_init(&filter);
    mp_filter_add_str(&filter, "role", "admin");

    mp_object_t results;
    mp_error_t err = mp_query_filter_array(&decoder, &zone, &filter, &results);
    
    if (err != MP_OK) append_error(test, "Filter failed on mixed array", err);
    if (results.via.array.size != 1) append_error(test, "Expected 1 admin found in mixed array", results.via.array.size);

    mp_zone_destroy(&zone);
    mp_memory_stream_destroy(&buffer);
    return test_end(test);
}

bool test_query_combined_execute_and_extract(test_result_t *test) {
    mp_zone_t zone;
    mp_zone_init(&zone, 4096);
    
    mp_object_t map;
    mp_build_map(&zone, &map, 1);
    
    mp_object_t arr;
    mp_build_array(&zone, &arr, 1);
    
    mp_object_t user;
    mp_build_map(&zone, &user, 3);
    mp_map_set_int(&user, 0, "id", 500);
    mp_map_set_str(&user, 1, "name", "Zoe");
    mp_map_set_str(&user, 2, "secret", "xxx");
    
    mp_array_set(&arr, 0, user);
    
    mp_object_t key;
    mp_build_cstr(&key, "data");
    map.via.map.ptr[0].key = key;
    map.via.map.ptr[0].val = arr;
    
    mp_stream_buffer_t buffer;
    mp_stream_t stream;
    mp_stream_init_write(&stream, &buffer, true, NULL, 0);
    mp_encode_object(&stream, &map);
    
    mp_stream_t read_stream;
    mp_stream_init_read(&read_stream, &buffer, buffer.data, buffer.size);
    mp_decoder_t decoder;
    mp_decoder_init(&decoder, &read_stream, NULL);
    
    // 1. Execute Path
    mp_query_t q;
    mp_query_init(&q);
    mp_query_add_path_str(&q, "data");
    mp_query_add_path_idx(&q, 0);
    
    mp_error_t err = mp_query_execute(&decoder, &q);
    if (err != MP_OK) append_error(test, "Execute path failed in combined test", err);
    
    // 2. Extract fields
    const char* fields[] = {"id", "secret"};
    mp_object_t out;
    err = mp_query_extract_fields(&decoder, &zone, fields, 2, &out);
    
    if (err != MP_OK) append_error(test, "Extract fields failed after execute path", err);
    if (out.via.map.size != 2) append_error(test, "Expected 2 extracted fields", out.via.map.size);

    mp_zone_destroy(&zone);
    mp_memory_stream_destroy(&buffer);
    return test_end(test);
}

test_suite_t suite_query = {.name = "MessagePack Query Suite",
                            .standard = "MsgPack-Query"};

int main(void) {
  r_set_global_verbosity(R_VERBOSE);
  enable_memleak_detection(&suite_query);

  add_test(&suite_query, test_query_map_key_str, "Query: Map Key String",
           "Query-Spec");
  add_test(&suite_query, test_query_array_index, "Query: Array Index",
           "Query-Spec");
  add_test(&suite_query, test_query_bad_args, "Query: Bad Arguments",
           "Query-Spec");
  add_test(&suite_query, test_query_type_mismatch, "Query: Type Mismatch",
           "Query-Spec");
  add_test(&suite_query, test_query_complex_map, "Query: Complex Map Keys",
           "Query-Spec");
  add_test(&suite_query, test_query_execute_path, "Query: Execute Path",
           "Query-Spec");
  add_test(&suite_query, test_query_extract_fields, "Query: Extract Fields",
           "Query-Spec");
  add_test(&suite_query, test_query_filter_array, "Query: Filter Array",
           "Query-Spec");
  add_test(&suite_query, test_query_long_string_keys, "Query: Long String Keys",
           "Query-Spec");
  add_test(&suite_query, test_query_filter_bool, "Query: Filter Boolean",
           "Query-Spec");
  add_test(&suite_query, test_query_max_limits, "Query: Max Limits",
           "Query-Spec");
  add_test(&suite_query, test_query_duplicate_keys_extraction, "Query: Duplicate Keys Extraction",
           "Query-Spec");
  add_test(&suite_query, test_query_truncated_payload, "Query: Truncated Payload",
           "Query-Spec");
  add_test(&suite_query, test_query_extract_missing_fields, "Query: Extract Missing Fields",
           "Query-Spec");
  add_test(&suite_query, test_query_filter_mixed_array, "Query: Filter Mixed Array",
           "Query-Spec");
  add_test(&suite_query, test_query_combined_execute_and_extract, "Query: Combined Execute & Extract",
           "Query-Spec");

  register_suite(&suite_query);
  bool success = run_all_suites();
  return success ? 0 : 1;
}
