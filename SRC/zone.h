#ifndef RADIKANT_MESSAGEPACK_ZONE_H
#define RADIKANT_MESSAGEPACK_ZONE_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct mp_zone_chunk {
    struct mp_zone_chunk* next;
    size_t size;
    size_t used;
    // Data follows immediately
} mp_zone_chunk_t;

typedef struct {
    mp_zone_chunk_t* head;
    size_t chunk_size;
} mp_zone_t;

// Initialize a zone with a default chunk size (e.g., 4096)
void mp_zone_init(mp_zone_t* zone, size_t chunk_size);

// Destroy the zone and free all memory chunks
void mp_zone_destroy(mp_zone_t* zone);

// Allocate size bytes from the zone
void* mp_zone_alloc(mp_zone_t* zone, size_t size);

#ifdef __cplusplus
}
#endif

#endif // RADIKANT_MESSAGEPACK_ZONE_H
