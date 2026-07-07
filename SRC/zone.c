#include "zone.h"
#include <stdlib.h>
#include <string.h>

#define MP_ZONE_ALIGN(size) (((size) + 7) & ~7)

void mp_zone_init(mp_zone_t* zone, size_t chunk_size) {
    zone->head = NULL;
    zone->chunk_size = chunk_size > 0 ? chunk_size : 4096;
}

void mp_zone_destroy(mp_zone_t* zone) {
    mp_zone_chunk_t* current = zone->head;
    while (current != NULL) {
        mp_zone_chunk_t* next = current->next;
        free(current);
        current = next;
    }
    zone->head = NULL;
}

void* mp_zone_alloc(mp_zone_t* zone, size_t size) {
    size_t aligned_size = MP_ZONE_ALIGN(size);
    mp_zone_chunk_t* current = zone->head;

    if (current && (current->size - current->used) >= aligned_size) {
        void* ptr = (char*)current + sizeof(mp_zone_chunk_t) + current->used;
        current->used += aligned_size;
        return ptr;
    }

    size_t new_chunk_size = zone->chunk_size;
    if (aligned_size > new_chunk_size) {
        new_chunk_size = aligned_size;
    }

    mp_zone_chunk_t* new_chunk = (mp_zone_chunk_t*)malloc(sizeof(mp_zone_chunk_t) + new_chunk_size);
    if (!new_chunk) {
        return NULL;
    }

    new_chunk->size = new_chunk_size;
    new_chunk->used = aligned_size;
    new_chunk->next = zone->head;
    zone->head = new_chunk;

    return (char*)new_chunk + sizeof(mp_zone_chunk_t);
}
