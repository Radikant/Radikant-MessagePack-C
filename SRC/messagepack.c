#include "messagepack.h"

mp_error_t mp_parse_memory(mp_zone_t* zone, const char* data, size_t size, mp_object_t* out_obj) {
    if (!zone || !data || !out_obj) return MP_ERROR_BAD_ARG;

    mp_stream_t stream;
    mp_memory_stream_ctx_t ctx;
    mp_memory_stream_init_read(&stream, &ctx, data, size);

    mp_decoder_t decoder;
    mp_decoder_init(&decoder, &stream, zone);

    return mp_decode(&decoder, out_obj);
}

mp_error_t mp_skip_memory(const char* data, size_t size) {
    if (!data) return MP_ERROR_BAD_ARG;

    mp_stream_t stream;
    mp_memory_stream_ctx_t ctx;
    mp_memory_stream_init_read(&stream, &ctx, data, size);

    mp_decoder_t decoder;
    mp_decoder_init(&decoder, &stream, NULL); // Zone not needed for skip

    return mp_skip(&decoder);
}
