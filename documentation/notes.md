# Architecture Notes: Streaming Search & Filtering

## The Problem
When working with an array of objects (e.g., `[{"name": "Charlie", "age": 37}, ...]`), finding an object that matches specific value criteria poses a unique challenge in a SAX-style parser. 

MessagePack streams are strictly **forward-only**. If you scan through an object in the stream to check its properties, you physically consume those bytes. If you reach the end of the map and realize it is a match, you cannot easily extract the full object because the stream cursor has already advanced past it.

To implement a "Filter" or "Search by Value" capability, we evaluated two architectural approaches.

---

## Approach A: "Parse & Discard"
This approach decodes the entire object into an Abstract Syntax Tree (AST) in memory. It then evaluates the AST against the filter conditions. If it matches, the AST is kept. If it fails, the memory is discarded, and the parser moves to the next object.

### Pros:
- **Universal Stream Support:** It works flawlessly across all stream types (Memory, Files, and live TCP Sockets) because it never looks backward. It maintains the pure forward-only streaming philosophy.
- **Instant Memory Wipes:** Because `Radikant-MessagePack-C` relies on the `mp_zone_t` Arena allocator, "discarding" the failed AST does not incur expensive `free()` calls. The library simply subtracts the bytes used from the zone's pointer (`zone->used = checkpoint`), making the memory wipe virtually zero-cost.

### Cons (The Fatal Flaw):
- **Astronomical CPU Overhead:** Imagine an array of 10,000 employees, and you are searching for one specific match. Approach A forces the CPU to build **9,999 complete AST trees** just to throw them away. The CPU must allocate nodes, parse MessagePack type tags, copy string bytes, and manage unions for tens of thousands of properties that you don't even care about.

---

## Approach B: "Stream Rewinding" (The Winner)
This approach extends the `mp_stream_t` interface to support `mark()` and `reset()` capabilities. The parser marks the start of the object in the stream, reads the keys, and aggressively uses `mp_skip()` on values it doesn't care about. If it finds the matching criteria, it rewinds the stream back to the mark and calls `mp_decode()` to build the AST.

### Pros:
- **Peak CPU Performance:** This is the absolute fastest path possible. For the 9,999 non-matching employees, the engine relies on `mp_skip()`. Skipping bypasses the entire object graph without building a single AST node. It simply calculates payload lengths and jumps pointers. The engine scans 9,999 failures at lightning speed, and builds exactly **1** AST tree.

### Cons:
- **Breaks Unbuffered Streams:** Rewinding requires the underlying stream to be navigable. This is trivial for memory buffers (`mp_stream_buffer_t`) where rewinding is just `cursor = saved_cursor`. It is possible for Files using `fseek()`. However, you **cannot** rewind a live, unbuffered TCP socket. 

---

## Conclusion
For maximum performance in `Radikant-MessagePack-C`, **Approach B (Stream Rewinding)** is the vastly superior choice. 

In real-world C applications, almost all MessagePack parsing occurs on in-memory buffers (e.g., a UDP packet loaded into a buffer, or an HTTP payload fully received). In the rare case of socket streaming, developers should load the chunk into memory before filtering it. 

The CPU savings of using `mp_skip()` over building and discarding 9,999 ASTs are too massive to ignore. Future architecture upgrades should focus on adding `mp_stream_mark` and `mp_stream_reset` to the underlying `mp_stream_t` abstraction to support native, high-speed filtering.
