#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <radikant-messagepack-c.h>

int main() {
    printf("==================================================\n");
    printf(" Radikant MessagePack-C | Developer Print Tools \n");
    printf("==================================================\n\n");

    mp_zone_t zone;
    mp_zone_init(&zone, 4096);

    // 1. Build a relatable structure: A company with 5 employees
    mp_object_t root;
    mp_build_map(&zone, &root, 2);
    
    mp_map_set_str(&root, 0, "company", "Radikant");

    mp_object_t employees_arr;
    mp_build_array(&zone, &employees_arr, 5);
    
    const char* names[] = {"Alice", "Bob", "Charlie", "David", "Eve"};
    const char* roles[] = {"CEO", "CTO", "Engineer", "Designer", "QA"};
    int ages[] = {42, 38, 29, 31, 26};

    for (int i = 0; i < 5; i++) {
        mp_object_t emp;
        mp_build_map(&zone, &emp, 3);
        mp_map_set_str(&emp, 0, "name", names[i]);
        mp_map_set_str(&emp, 1, "role", roles[i]);
        mp_map_set_int(&emp, 2, "age", ages[i]);
        mp_array_set(&employees_arr, i, emp);
    }

    mp_object_t emp_key;
    mp_build_cstr(&emp_key, "employees");
    mp_map_set(&root, 1, emp_key, employees_arr);

    // 2. Print the AST beautifully
    printf("--- [AST: Formatted JSON Output] ---\n");
    mp_print_object(&root);
    printf("------------------------------------\n\n");

    // 3. Encode to Binary
    mp_stream_buffer_t buffer;
    mp_stream_t enc_stream;
    mp_stream_init_write(&enc_stream, &buffer, true, NULL, 0);
    mp_encode_object(&enc_stream, &root);

    // 4. Print the raw binary Hex Dump
    printf("--- [STREAM: Raw Hex Dump Output] ---\n");
    mp_print_hex_dump(buffer.data, buffer.size);
    printf("-------------------------------------\n\n");

    mp_zone_destroy(&zone);
    mp_memory_stream_destroy(&buffer);
    return 0;
}
