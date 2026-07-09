# Architecture

Radikant-MessagePack-C is designed as a set of focused, highly decoupled building blocks. You only pay for the features you use.

## Core Building Blocks

### 1. Abstract I/O (`stream.h`)
Everything reads and writes through an abstract `mp_stream_t` interface. Whether you are serializing to a stack-allocated buffer, a dynamic heap buffer, or streaming directly to a TCP socket, the core library doesn't care.

### 2. Memory Management (`zone.h`)
Instead of making thousands of tiny `malloc` calls when parsing complex objects, the library uses an Arena Allocator (`mp_zone_t`). All memory is allocated in large chunks and freed instantly in a single call when the zone is destroyed.

### 3. The Object Model (`object.h`, `builder.h`)
This is the Abstract Syntax Tree (AST). `mp_object_t` represents any MessagePack type. The Builder API provides safe helpers to construct nested structures (Arrays and Maps) within a memory zone.

### 4. Serialization (`encoder.h`, `decoder.h`)
- **Encoder**: Takes primitive C types or full `mp_object_t` ASTs and packs them into the stream.
- **Decoder**: Parses streams. It can either build a full `mp_object_t` tree in memory, or it can act as a **Streaming Parser** (SAX), reading primitive values sequentially without allocating any nodes.

### 5. High-Performance Tooling
- **Fast-Forwarding (`skip.h`)**: Allows the decoder to instantly jump over massive nested structures in the stream without parsing them into memory.
- **Data Extraction (`query.h`)**: Provides a path-based querying language (like JSONPath) to pluck specific nested keys or array indices directly from a stream.
