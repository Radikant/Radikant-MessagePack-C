# Radikant-MessagePack-C

A [MessagePack](https://msgpack.org/) implementation in C.

## Summary

Zero-dependency MessagePack encoder/decoder in C with arena allocation, AST building, streaming (SAX) parsing, zero-alloc element skipping, and JSONPath-style queries. Performance sits between [CMP](https://github.com/camgunz/cmp) and [MPack](https://github.com/ludocode/mpack) — faster than CMP on all benchmarks, competitive with MPack, and outperforms both on difficult-vector decoding (778 MB/s vs MPack's 648 MB/s). Zero-alloc skipping reaches 1024 MB/s, 3.7× faster than CMP.

## Get

```bash
git clone --recurse-submodules https://github.com/Radikant/Radikant-MessagePack-C
```

## Usage

```c
#include "radikant-messagepack-c.h"
```

### Encode

```c
mp_zone_t zone;
mp_zone_init(&zone, 4096);

// Build an array: [42, "Hello"]
mp_object_t array, item;
mp_build_array(&zone, &array, 2);

mp_build_int(&item, 42);
mp_array_set(&array, 0, item);

mp_build_cstr(&item, "Hello");
mp_array_set(&array, 1, item);

// Serialize to buffer
char *data = NULL;
size_t size = 0;
mp_serialize_memory(&array, &data, &size);
```

### Decode

```c
// Parse buffer back into an AST
mp_object_t result;
mp_parse_memory(&zone, data, size, &result);

// Access values
int64_t  num = result.via.array.ptr[0].via.u64;
const char *str = result.via.array.ptr[1].via.str.ptr;

// Cleanup
free(data);
mp_zone_destroy(&zone);
```

## Documentation

| Module | Description |
|---|---|
| [Architecture](documentation/architecture.md) | System overview and design |
| [Zone](documentation/zone.md) | Arena memory allocator |
| [Object](documentation/object.md) | AST node types |
| [Builder](documentation/builder.md) | Object construction helpers |
| [Streamer](documentation/streamer.md) | I/O stream abstraction |
| [Query](documentation/query.md) | JSONPath-style queries |
| [Skip](documentation/skip.md) | Zero-parse element skipping |
| [Notes](documentation/notes.md) | Implementation notes |

## Examples

See the [example/](example/) folder for complete, runnable programs.

## Testing

[Radikant-Probe-C](https://github.com/Radikant/Radikant-Probe-C) is used for testing. See the [test/](test/) folder to explore.

## License

Radikant License © 2026 [Radikant](https://github.com/Radikant)
