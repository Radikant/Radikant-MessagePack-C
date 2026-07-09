# Advanced Query API

The Query API provides high-performance utility functions to search through MessagePack streams for specific elements, often without allocating memory for an Abstract Syntax Tree (AST).

## Core Capabilities
1. **Deep Path Queries (`mp_query_execute`)**: Navigate a multi-level JSON-like path instantly to position the decoder at a nested element.
2. **Array Filtering (`mp_query_filter_array`)**: Scan arrays for maps that match specific criteria (e.g., `role == "admin"`) and pluck them into an AST, instantly discarding the rest.
3. **Field Extraction (`mp_query_extract_fields`)**: Pluck only a subset of keys from a large map into an AST, ignoring everything else.
4. **Single-Element Navigation (`mp_query_map_key_str`, `mp_query_array_index`)**: The lowest-level primitive; jump instantly to a specific map key or array index without allocating memory.

## Stream Rewinding & Re-initialization
> [!WARNING]
> **Stream Consumption:** Because the Query API operates on raw streams, searching consumes bytes. You cannot easily rewind the stream after doing complex array filters or query executions. To execute a completely different query on the same payload, it is highly recommended to **re-initialize a fresh zero-allocation decoder** pointing to the exact same buffer.

---

## 1. Deep Path Queries
Use `mp_query_t` to declare a traversal path, then execute it. The decoder is left positioned exactly at the target element.

```c
// Example: Drill into `data -> users -> [1]`
mp_query_t q;
mp_query_init(&q);
mp_query_add_path_str(&q, "data");
mp_query_add_path_str(&q, "users");
mp_query_add_path_idx(&q, 1);

if (mp_query_execute(&decoder, &q) == MP_OK) {
    // The decoder is now perfectly positioned at the 2nd user!
    // You can now decode it into an AST or use the SAX streaming API.
}
```

---

## 2. Array Filtering
The `mp_filter_t` API enables you to instantly search a massive array of maps for elements matching certain criteria (e.g. searching for a user ID or role). It operates with zero heap allocations until a match is found.

```c
mp_filter_t filter;
mp_filter_init(&filter);
mp_filter_add_str(&filter, "role", "admin");
// Optional: mp_filter_add_int(&filter, "age", 35); // Supports multiple AND conditions

mp_object_t admins_ast;
if (mp_query_filter_array(&decoder, &zone, &filter, &admins_ast) == MP_OK) {
    // admins_ast is now an array containing ONLY the matching objects!
}
```

---

## 3. Surgical Field Extraction
Instead of parsing an entire Map object into an AST (which allocates memory for every single key-value pair), use `mp_query_extract_fields` to pluck only the specific keys you care about.

```c
// Assuming the decoder is positioned at a massive Map...
const char* fields[] = {"id", "role"};
mp_object_t extracted_ast;

if (mp_query_extract_fields(&decoder, &zone, fields, 2, &extracted_ast) == MP_OK) {
    // extracted_ast is a Map containing ONLY "id" and "role".
}
```

---

## 4. Single-Element Zero-Allocation Navigation
For maximum performance and memory efficiency, you can execute simple jumps manually using `mp_query_map_key_str` or `mp_query_array_index`.

```c
// Jump directly to the "status" key in the current Map
if (mp_query_map_key_str(&decoder, "status") == MP_OK) {
    
    // Decode the string manually via SAX streaming API (No AST allocations!)
    uint32_t len;
    if (mp_decode_stream_str_len(&decoder.stream, &len) == MP_OK) {
        char status_buf[64];
        if (len < sizeof(status_buf)) {
            decoder.stream.read(&decoder.stream, status_buf, len);
            status_buf[len] = '\0';
            printf("Found status: %s\n", status_buf);
        }
    }
}
```
