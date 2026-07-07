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

// Skips a complete MessagePack payload without allocating any memory.
mp_error_t mp_skip_memory(const char* data, size_t size);

#ifdef __cplusplus
}
#endif

#endif // RADIKANT_MESSAGEPACK_SKIP_H
