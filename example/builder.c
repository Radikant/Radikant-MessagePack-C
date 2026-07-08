#include <stdio.h>
#include <stdlib.h>

#include <radikant-messagepack-c.h>

void encode_builder_example(mp_zone_t *zone, char **out_data, size_t *out_size) {
    // 1. Create a root dictionary/map AST
    mp_object_t user_map;
    mp_build_map(zone, &user_map, 3);
    
    mp_map_set_str(&user_map, 0, "name", "Charlie");
    mp_map_set_int(&user_map, 1, "age", 29);
    
    // 2. We can build complex objects in pieces and attach them later
    mp_object_t roles_array;
    mp_build_array(zone, &roles_array, 2);
    
    mp_object_t role;
    mp_build_cstr(&role, "Admin");
    mp_array_set(&roles_array, 0, role);
    
    mp_build_cstr(&role, "Developer");
    mp_array_set(&roles_array, 1, role);
    
    // Attach the array to the map
    mp_object_t key;
    mp_build_cstr(&key, "roles");
    mp_map_set(&user_map, 2, key, roles_array);
    
    // ---------------------------------------------------------
    // 3. The true power of the Builder API: MUTATION!
    // ---------------------------------------------------------
    // Because we built an AST in memory, we can traverse it and 
    // modify it BEFORE we serialize it to binary.
    printf("[BUILDER] Original age: %lld\n", (long long)user_map.via.map.ptr[1].val.via.u64);
    
    // Let's change the user's age from 29 to 30.
    mp_build_int(&user_map.via.map.ptr[1].val, 30);
    printf("[BUILDER] Mutated age to: 30\n");
    
    // 4. Serialize the entire tree at once to memory
    mp_error_t err = mp_serialize_memory(&user_map, out_data, out_size);
    if (err == MP_OK) {
        printf("[BUILDER] Serialized AST into %zu bytes.\n\n", *out_size);
    }
}

void decode_builder_example(mp_zone_t *zone, const char *data, size_t size) {
    mp_object_t ast;
    mp_error_t err = mp_parse_memory(zone, data, size, &ast);
    
    if (err == MP_OK) {
        printf("[DECODE] Successfully parsed AST from binary data.\n");
        // Prove the age mutation was successfully encoded and decoded
        if (ast.type == MP_TYPE_MAP && ast.via.map.size >= 2) {
            mp_object_str_t name = ast.via.map.ptr[0].val.via.str;
            uint64_t age = ast.via.map.ptr[1].val.via.u64;
            
            printf("[DECODE] Found Name: %.*s\n", (int)name.size, name.ptr);
            printf("[DECODE] Found Age : %llu\n", (unsigned long long)age);
        }
    }
}

int main() {
    printf("=========================================\n");
    printf(" Radikant MessagePack-C Builder Example\n");
    printf("=========================================\n\n");
    
    mp_zone_t zone;
    mp_zone_init(&zone, 4096);
    
    char *binary_data = NULL;
    size_t size = 0;
    
    encode_builder_example(&zone, &binary_data, &size);
    
    if (binary_data) {
        decode_builder_example(&zone, binary_data, size);
        free(binary_data); // Free the buffer returned by mp_serialize_memory
    }
    
    mp_zone_destroy(&zone);
    
    return 0;
}
