# Zone Allocator

The Zone API (`mp_zone_t`) is a fast, linear memory allocator (often called an arena allocator) used internally by the **Builder API** to allocate Abstract Syntax Tree (AST) nodes.

## Why use it?
- **Blazing Fast Allocations**: Allocating memory is as simple as bumping a pointer within a pre-allocated chunk. No complex `malloc` overhead per object.
- **Batched Deallocation**: You don't free individual objects. Instead, you destroy the entire zone at once (`mp_zone_destroy`), preventing memory leaks and bypassing the overhead of `free()`.
- **Cache Friendly**: Memory is allocated contiguously in large chunks, which vastly improves CPU cache hit rates.

## How it works
The zone allocates large blocks of memory (chunks) from the system. As you request memory via `mp_zone_alloc`, it simply increments an offset within the current chunk and returns the pointer. If the chunk fills up, it allocates a new one and links them together.

## Example Usage

```c
#include <radikant-messagepack-c.h>
#include <stdio.h>

void zone_example() {
    mp_zone_t zone;
    
    // 1. Initialize a zone with a default chunk size (e.g., 4096 bytes)
    mp_zone_init(&zone, 4096);

    // 2. Perform fast allocations without malloc overhead
    char* str1 = mp_zone_alloc(&zone, 32);
    char* str2 = mp_zone_alloc(&zone, 128);
    
    // Note: Usually, you pass the zone into Builder API functions 
    // rather than calling mp_zone_alloc directly.
    // mp_build_array(&zone, &array, 3);
    
    // 3. Destroying the zone instantly frees ALL allocations made within it
    mp_zone_destroy(&zone);
}
```
