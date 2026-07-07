#ifndef RADIKANT_MESSAGEPACK_SKIP_H
#define RADIKANT_MESSAGEPACK_SKIP_H

#include "types.h"
#include "error.h"
#include "decoder.h"

#ifdef __cplusplus
extern "C" {
#endif

// Skips the next MessagePack object in the decoder stream without allocating memory.
mp_error_t mp_skip(mp_decoder_t* decoder);

#ifdef __cplusplus
}
#endif

#endif // RADIKANT_MESSAGEPACK_SKIP_H
