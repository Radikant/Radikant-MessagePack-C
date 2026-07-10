# Decoder Performance Analysis

## Current Gap

| Vector | MPack (MB/s) | Radikant (MB/s) | Gap |
|---|---|---|---|
| Simple | 868 | 594 | **-32%** |
| Hard | 1,458 | 1,019 | **-30%** |
| Difficult | 3,644 | 3,379 | **-7%** |

Radikant is ~30% behind mpack on simple/hard vectors. The gap shrinks on the difficult vector because its structural complexity amortizes the fixed overhead identified below.

---

## Root Cause Analysis

### 1. `read_bytes()` Indirection — THE BIGGEST ISSUE

Every single byte read in `mp_decode_internal` goes through [read_bytes()](file:///Users/charrlie/Desktop/Radikant-MessagePack-C/SRC/decoder.c#L17-L32):

```c
static inline mp_error_t read_bytes(mp_decoder_t *decoder, void *buf, size_t len) {
  if (len == 0) return MP_OK;                    // branch 1
  mp_stream_t *stream = decoder->stream;
  if (!stream) return MP_ERROR_BAD_ARG;           // branch 2
  if (stream->fast_left >= len) {                 // branch 3
      memcpy(buf, stream->fast_ptr, len);         // memcpy into stack buf
      stream->fast_ptr += len;
      stream->fast_left -= len;
      if (stream->fast_size_ptr) *(stream->fast_size_ptr) += len;  // branch 4
      return MP_OK;
  }
  if (!stream->read) return MP_ERROR_BAD_ARG;     // branch 5
  return stream->read(stream, buf, len);          // function pointer call
}
```

**Per-element cost**: reading a uint32 requires:
1. `read_bytes(decoder, &b, 1)` — read tag byte into stack var (4 branches + memcpy of 1 byte)
2. `read_bytes(decoder, buf, 4)` — read payload into stack buf (4 branches + memcpy of 4 bytes)
3. `deserialize_be32(buf)` — bswap from the stack buffer

**How mpack does it**: Direct pointer arithmetic. Zero function calls, zero branches, zero memcpy:
```c
uint8_t type = mpack_load_u8(tree->data + tree->size);      // direct load
node->value.u = mpack_load_u32(tree->data + tree->size + 1); // direct load + bswap
```

MPack maintains a single `size` offset into the data buffer. It validates bounds once per node upfront via `mpack_tree_reserve_bytes`, then reads directly. No per-read branching.

> **Impact**: For the hard vector (10,000 elements), Radikant executes ~20,000+ `read_bytes` calls with ~80,000 branches. MPack executes zero read calls — just pointer dereferences.

---

### 2. Recursive vs Iterative Parsing

Radikant uses recursion ([mp_decode_internal](file:///Users/charrlie/Desktop/Radikant-MessagePack-C/SRC/decoder.c#L110-L447)):
```c
for (uint32_t i = 0; i < len; ++i) {
    decoder->depth++;
    err = mp_decode_internal(decoder, &out_obj->via.array.ptr[i]);
    decoder->depth--;
    if (err != MP_OK) return err;
}
```

MPack uses an explicit stack with an iterative loop ([mpack_tree_continue_parsing](file:///Users/charrlie/Desktop/Radikant-MessagePack-C/lib/mpack/src/mpack/mpack-node.c#L792-L825)):
```c
while (true) {
    node = parser->stack[parser->level].child;
    mpack_tree_parse_node(tree, node);
    --parser->stack[level].left;
    ++parser->stack[level].child;
    while (parser->stack[parser->level].left == 0) {
        if (parser->level == 0) return true;
        --parser->level;
    }
}
```

**Why this matters**:
- Each recursive call pushes a stack frame (~64-128 bytes: saved registers, return address, local vars)
- The hard vector has arrays nested inside arrays — recursion depth = 2, but 10,000 elements means 10,000 function call/return pairs
- Iterative loop keeps everything in registers and avoids call/ret overhead
- The `depth++`/`depth--` pair is also redundant bookkeeping per element

---

### 3. `mp_object_t` is 24 Bytes — Double MPack's Node

```c
struct mp_object_t {
    mp_type_t type;              // 4 bytes (enum)
    union mp_object_union_t via; // 16 bytes (largest member: ext with type + size + ptr)
};  // Total: 24 bytes (with padding)
```

MPack's `mpack_node_data_t` is ~16 bytes. For the hard vector (10,000 elements), Radikant allocates **240KB** of AST nodes vs mpack's ~160KB. More memory = more cache misses = slower.

`mp_object_kv_t` is **48 bytes** (two `mp_object_t`). A map with 100 keys allocates 4,800 bytes just for the KV pairs.

---

### 4. Per-Element Zone Allocation Overhead

For arrays/maps, Radikant calls `mp_zone_alloc()` per container to allocate children. The zone allocator itself is fast (pointer bump), but:
- It aligns every allocation to 8 bytes (`MP_ZONE_ALIGN`)
- It checks `current->size - current->used >= aligned_size` every call
- If the chunk is full, it calls `malloc()` for a new chunk

MPack pre-allocates a **node page pool** and just bumps a pointer. When children are needed ([mpack_tree_parse_children](file:///Users/charrlie/Desktop/Radikant-MessagePack-C/lib/mpack/src/mpack/mpack-node.c#L269-L310)):
```c
if (total <= parser->nodes_left) {
    node->value.children = parser->nodes;  // just take the next slots
    parser->nodes += total;
    parser->nodes_left -= total;
}
```

No alignment, no size checks, no branching for common case. This is a simpler/faster bump allocator than the zone.

---

### 5. `fast_size_ptr` Update on Every Read

Inside `read_bytes`, there's a conditional write on every single read:
```c
if (stream->fast_size_ptr) *(stream->fast_size_ptr) += len;
```

This updates `buffer->offset` through a pointer. The branch itself is cheap (always taken for memory streams), but the store adds a data dependency and prevents the compiler from keeping `fast_ptr`/`fast_left` in registers across calls.

---

## Proposed Optimizations

### P0: Direct-Access Memory Decoder (Eliminates issues 1, 2, 5)

Add a specialized `mp_parse_memory_fast()` that operates directly on the buffer with pointer arithmetic — no stream abstraction, no `read_bytes()`, no function calls per element. This is what mpack's tree parser does.

```c
// Conceptual sketch — replaces read_bytes entirely for memory buffers
typedef struct {
    const char *ptr;
    const char *end;
    mp_zone_t *zone;
    uint32_t depth;
    uint32_t max_depth;
} mp_fast_decoder_t;

static mp_error_t mp_fast_decode(mp_fast_decoder_t *d, mp_object_t *out) {
    if (d->ptr >= d->end) return MP_ERROR_DECODE_INCOMPLETE;
    uint8_t b = *(const uint8_t *)d->ptr++;

    if (b <= 0x7f) {
        out->type = MP_TYPE_POSITIVE_INTEGER;
        out->via.u64 = b;
        return MP_OK;
    }
    // ... direct reads with d->ptr, no function calls
}
```

**Expected impact**: ~25-30% improvement, closes most of the gap to mpack.

### P1: Iterative Loop (Eliminates issue 2)

Convert the recursive `mp_decode_internal` loop for arrays/maps to an explicit stack, similar to mpack. Eliminates function call overhead per element.

### P2: Flatten Node Pools (Reduces issue 3, 4)

Instead of per-container `mp_zone_alloc()`, pre-calculate the total node count with a fast scan pass, allocate once, then fill. This eliminates per-container allocation overhead and improves cache locality. The tradeoff is a two-pass decode.

### P3: Compact `mp_object_t` (Reduces issue 3)

Consider whether `mp_type_t` can be a `uint8_t` instead of an `int` enum (saves 3 bytes padding). Or pack the type into the union's unused bits. Reduces per-node size from 24 to potentially 16 bytes.

---

## Priority Order

| Priority | Optimization | Effort | Expected Impact |
|---|---|---|---|
| **P0** | Direct-access memory decoder | Medium | **+25-30%** |
| **P1** | Iterative parsing loop | Low | **+5-10%** |
| **P2** | Pre-allocated node pool | Medium | **+3-5%** |
| **P3** | Compact `mp_object_t` | Low | **+2-3%** |

P0 alone should close most of the gap. P0+P1 combined could match or beat mpack.
