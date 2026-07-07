#ifndef RADIKANT_MESSAGEPACK_DECODER_H
#define RADIKANT_MESSAGEPACK_DECODER_H

#include "types.h"
#include "error.h"
#include "zone.h"
#include "object.h"
#include "stream.h"
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    mp_stream_t* stream;
    mp_zone_t* zone;
    uint32_t depth;
    uint32_t max_depth;
} mp_decoder_t;

void mp_decoder_init(mp_decoder_t* decoder, mp_stream_t* stream, mp_zone_t* zone);
mp_error_t mp_decode(mp_decoder_t* decoder, mp_object_t* out_obj);

#ifdef __cplusplus
}
#endif

#endif // RADIKANT_MESSAGEPACK_DECODER_H
