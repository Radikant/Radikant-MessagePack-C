# Object API

The Object API (`mp_object_t`) is a tagged union that represents a single node of data (such as an integer, string, array, or map). It is the fundamental building block of the **Builder API** and behaves exactly like a node in a JSON Document Object Model (DOM).

## Why is it in this library?
If you only had the Streaming API, you would be forced to process all data strictly in order as raw primitives. The Object API exists so you can:
- **Build Trees**: Construct and manipulate complex, nested structures (Abstract Syntax Trees) entirely in memory.
- **Random Access**: Query arrays and maps by index out of order.
- **Dynamic Typing**: Safely inspect the type of a value at runtime (e.g., determining if a field is a string, boolean, or integer before attempting to read it).

## How is it used?
An `mp_object_t` is either built manually (via `mp_build_*` functions) to be later serialized, or it is returned by the decoder (`mp_decode`) after parsing a binary stream. Once you have an object, you extract its underlying values using safe accessor functions (`mp_object_as_*`).

### Example: Safely Reading an Object

```c
#include <radikant-messagepack-c.h>
#include <stdio.h>

void process_object(const mp_object_t* obj) {
    if (!obj) return;

    // 1. You can inspect the type directly
    if (obj->type == MP_TYPE_STR) {
        printf("I found a string!\n");
    }

    // 2. Or use safe accessors to extract data without crashing
    int64_t int_val;
    if (mp_object_as_int(obj, &int_val) == MP_OK) {
        printf("Value is an integer: %lld\n", (long long)int_val);
    } else {
        printf("Value is NOT an integer!\n");
    }

    // 3. Accessing nested collections (Arrays/Maps)
    if (obj->type == MP_TYPE_ARRAY) {
        printf("Array has %u elements.\n", obj->via.array.size);
        
        // Grab the first element safely
        mp_object_t* first_element = mp_array_get(obj, 0);
        if (first_element) {
            process_object(first_element); // Process recursively
        }
    }
}
```
