#ifndef RADIKANT_MESSAGEPACK_H
#define RADIKANT_MESSAGEPACK_H

#ifdef __cplusplus
extern "C" {
#endif

#include "types.h"
#include "error.h"
#include "zone.h"
#include "object.h"
#include "encoder.h"
#include "decoder.h"
#include "skip.h"

// Top-Level Convenience APIs

// Parses a complete MessagePack payload from a memory buffer into the given root object.
// Memory for strings, bins, and arrays/maps will be allocated from `zone`.
mp_error_t mp_parse_memory(mp_zone_t* zone, const char* data, size_t size, mp_object_t* out_obj);

// Skips a complete MessagePack payload without allocating any memory.
mp_error_t mp_skip_memory(const char* data, size_t size);

#ifdef __cplusplus
}
#endif

#endif // RADIKANT_MESSAGEPACK_H
