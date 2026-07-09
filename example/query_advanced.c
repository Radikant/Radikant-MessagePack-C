#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <radikant-messagepack-c.h>
#include <tools/print.h>
int encode_massive_payload(mp_zone_t *zone, mp_stream_buffer_t *out_buffer) {
  mp_object_t root_map;
  mp_build_map(zone, &root_map, 2);
  mp_map_set_str(&root_map, 0, "status", "success");
  mp_object_t data_map;
  mp_build_map(zone, &data_map, 1);
  mp_object_t users_arr;
  mp_build_array(zone, &users_arr, 4);
  mp_object_t user0;
  mp_build_map(zone, &user0, 5); 
  mp_map_set_int(&user0, 0, "id", 101);
  mp_map_set_str(&user0, 1, "name", "Alice");
  mp_map_set_str(&user0, 2, "secret_token", "xxx");
  mp_map_set_str(&user0, 3, "role", "admin");
  mp_map_set_int(&user0, 4, "salary", 95000);
  mp_array_set(&users_arr, 0, user0);
  
  mp_object_t user1;
  mp_build_map(zone, &user1, 5);
  mp_map_set_int(&user1, 0, "id", 102);
  mp_map_set_str(&user1, 1, "name", "Charlie");
  mp_map_set_str(&user1, 2, "secret_token", "yyy");
  mp_map_set_str(&user1, 3, "role", "editor");
  mp_map_set_int(&user1, 4, "salary", 80000);
  mp_array_set(&users_arr, 1, user1);

  mp_object_t user2;
  mp_build_map(zone, &user2, 5);
  mp_map_set_int(&user2, 0, "id", 103);
  mp_map_set_str(&user2, 1, "name", "Bob");
  mp_map_set_str(&user2, 2, "secret_token", "zzz");
  mp_map_set_str(&user2, 3, "role", "admin");
  mp_map_set_int(&user2, 4, "salary", 91000);
  mp_array_set(&users_arr, 2, user2);

  mp_object_t user3;
  mp_build_map(zone, &user3, 5);
  mp_map_set_int(&user3, 0, "id", 104);
  mp_map_set_str(&user3, 1, "name", "Diana");
  mp_map_set_str(&user3, 2, "secret_token", "aaa");
  mp_map_set_str(&user3, 3, "role", "viewer");
  mp_map_set_int(&user3, 4, "salary", 50000);
  mp_array_set(&users_arr, 3, user3);
  
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

// ---------------------------------------------------------
// CONCEPT 1: Query Paths (Deep Plucking)
// Let's instantly drill down to: data -> users
// ---------------------------------------------------------
mp_error_t demonstrate_query_path(mp_decoder_t* decoder) {
    printf("[CONCEPT 1: QUERY PATH]\nExecuting Path: 'data' -> 'users'\n");
    mp_query_t q;
    mp_query_init(&q);
    mp_query_add_path_str(&q, "data");
    mp_query_add_path_str(&q, "users");
    
    mp_error_t err = mp_query_execute(decoder, &q);
    if (err != MP_OK) {
        printf("[ERROR] Failed to execute query path: %d\n", err);
        return err;
    }
    printf("[SUCCESS] Decoder successfully positioned at the 'users' array!\n\n");
    return MP_OK;
}

// ---------------------------------------------------------
// CONCEPT 2: Array Filtering (Zero-Allocation Search)
// Scan the array for any users where role == "admin"
// ---------------------------------------------------------
mp_error_t demonstrate_array_filtering(mp_decoder_t* decoder, mp_zone_t* zone) {
    printf("[CONCEPT 2: ARRAY FILTERING]\nScanning array for users where 'role' == 'admin'...\n");
    mp_filter_t filter;
    mp_filter_init(&filter);
    mp_filter_add_str(&filter, "role", "admin");

    mp_object_t admins_ast;
    mp_error_t err = mp_query_filter_array(decoder, zone, &filter, &admins_ast);
    if (err == MP_OK && admins_ast.type == MP_TYPE_ARRAY) {
        printf("[SUCCESS] Found %u admins! Printing AST:\n", admins_ast.via.array.size);
        mp_print_object(&admins_ast);
        printf("\n");
    } else {
        printf("[ERROR] Filter failed: %d\n", err);
    }
    printf("\n");
    return err;
}

// ---------------------------------------------------------
// CONCEPT 3: Stream Re-initialization & Field Extraction
// Because `mp_query_filter_array` uses the stream's mark/reset internally
// to achieve zero-allocation scanning, our previous mark was overwritten!
// To perform another query, we simply initialize a fresh zero-cost decoder.
// ---------------------------------------------------------
mp_error_t demonstrate_field_extraction(const char* data, size_t size, mp_zone_t* zone) {
    printf("[CONCEPT 3: FRESH STREAM & EXTRACT]\nInitializing fresh decoder to start over...\n");
    
    mp_stream_t stream;
    mp_stream_buffer_t read_buf;
    mp_stream_init_read(&stream, &read_buf, data, size);
    
    mp_decoder_t decoder;
    mp_decoder_init(&decoder, &stream, NULL);

    printf("Drilling into 'data' -> 'users' -> [1]...\n");
    mp_query_t q;
    mp_query_init(&q);
    mp_query_add_path_str(&q, "data");
    mp_query_add_path_str(&q, "users");
    mp_query_add_path_idx(&q, 1);
    
    mp_error_t err = mp_query_execute(&decoder, &q);
    if (err != MP_OK) {
        printf("[ERROR] Failed to drill to index 1: %d\n", err);
        return err;
    }

    printf("Extracting only fields: 'id', 'role' from user[1] into AST...\n");
    const char* fields[] = {"id", "role"};
    mp_object_t user_ast;
    err = mp_query_extract_fields(&decoder, zone, fields, 2, &user_ast);
    if (err == MP_OK) {
        printf("[SUCCESS] Extracted Map Size: %u fields (ignored the rest!)\n", user_ast.via.map.size);
        mp_print_object(&user_ast);
        printf("\n");
    } else {
        printf("[ERROR] Failed to extract fields: %d\n", err);
    }
    return err;
}

// ---------------------------------------------------------
// CONCEPT 4: Zero-Allocation Single Key Query (SAX Mode)
// Find the exact value of a key in a map without allocating an AST!
// ---------------------------------------------------------
mp_error_t demonstrate_zero_allocation_query(const char* data, size_t size) {
    printf("[CONCEPT 4: ZERO-ALLOCATION KEY QUERY]\nInitializing fresh decoder to scan root map for 'status'...\n");
    
    mp_stream_t stream;
    mp_stream_buffer_t read_buf;
    mp_stream_init_read(&stream, &read_buf, data, size);
    mp_decoder_t decoder;
    mp_decoder_init(&decoder, &stream, NULL); // No zone needed!
    
    mp_error_t err = mp_query_map_key_str(&decoder, "status");
    if (err == MP_OK) {
        // Decoder is perfectly positioned at the value! Read it directly via SAX API.
        char status_buf[64];
        uint32_t len;
        err = mp_decode_stream_str_len(&stream, &len);
        if (err == MP_OK) {
            if (len >= sizeof(status_buf)) len = sizeof(status_buf) - 1;
            err = stream.read(&stream, status_buf, len);
            if (err == MP_OK) {
                status_buf[len] = '\0';
                printf("[SUCCESS] Found 'status': %s\n", status_buf);
            } else {
                printf("[ERROR] Failed to read string bytes: %d\n", err);
            }
        } else {
            printf("[ERROR] Failed to read status value: %d\n", err);
        }
    } else {
        printf("[ERROR] Failed to find 'status' key: %d\n", err);
    }

    printf("\n==================================================\n");
    return err;
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
    printf("[ENCODE] Generated huge nested payload with 4 users.\n\n");

    // 2. Setup the primary stream decoder for Concepts 1 & 2
    mp_stream_buffer_t read_buf;
    mp_stream_t stream;
    mp_stream_init_read(&stream, &read_buf, buffer.data, buffer.size);
    mp_decoder_t decoder;
    mp_decoder_init(&decoder, &stream, NULL); // No zone needed yet!

    // Run the demonstrations
    if (demonstrate_query_path(&decoder) == MP_OK) {
        if (demonstrate_array_filtering(&decoder, &zone) != MP_OK) {
            printf("[WARN] Array filtering demonstration failed.\n");
        }
    } else {
        printf("[WARN] Skipping array filtering since query path failed.\n");
    }
    
    if (demonstrate_field_extraction(buffer.data, buffer.size, &zone) != MP_OK) {
        printf("[WARN] Field extraction demonstration failed.\n");
    }
    
    if (demonstrate_zero_allocation_query(buffer.data, buffer.size) != MP_OK) {
        printf("[WARN] Zero-allocation query demonstration failed.\n");
    }

    // Cleanup
    mp_zone_destroy(&zone);
    mp_memory_stream_destroy(&buffer);
    return 0;
}
