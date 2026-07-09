#include <stdio.h>
#include <string.h>
#include <cmp.h>
#include <mpack.h>
#include <radikant-messagepack-c.h>

static size_t local_cmp_writer(cmp_ctx_t *ctx, const void *data, size_t count) {
    char **buf = (char**)ctx->buf;
    memcpy(*buf, data, count);
    *buf += count;
    return count;
}

int main() {
    char buf_radikant[1024];
    char buf_cmp[1024];
    char buf_mpack[1024];

    mp_stream_t stream_rad;
    mp_stream_buffer_t mem_rad;
    mp_stream_init_write(&stream_rad, &mem_rad, false, buf_radikant, sizeof(buf_radikant));
    mp_encode_array_len(&stream_rad, 4);
    mp_encode_bool(&stream_rad, true);
    mp_encode_int(&stream_rad, -42);
    mp_encode_double(&stream_rad, 3.14);
    mp_encode_str(&stream_rad, "hello", 5);

    char *cmp_ptr = buf_cmp;
    cmp_ctx_t cmp;
    cmp_init(&cmp, &cmp_ptr, NULL, NULL, local_cmp_writer);
    cmp_write_array(&cmp, 4);
    cmp_write_bool(&cmp, true);
    cmp_write_sint(&cmp, -42);
    cmp_write_decimal(&cmp, 3.14);
    cmp_write_str(&cmp, "hello", 5);

    mpack_writer_t mpack;
    mpack_writer_init(&mpack, buf_mpack, sizeof(buf_mpack));
    mpack_start_array(&mpack, 4);
    mpack_write_bool(&mpack, true);
    mpack_write_i32(&mpack, -42);
    mpack_write_double(&mpack, 3.14);
    mpack_write_str(&mpack, "hello", 5);
    mpack_finish_array(&mpack);

    size_t rad_size = mem_rad.offset;
    size_t cmp_size = cmp_ptr - buf_cmp;
    size_t mpack_size = mpack_writer_buffer_used(&mpack);

    printf("rad_size: %zu\n", rad_size);
    printf("cmp_size: %zu\n", cmp_size);
    printf("mpack_size: %zu\n", mpack_size);

    printf("rad: "); for (size_t i=0; i<rad_size; i++) printf("%02x ", (unsigned char)buf_radikant[i]); printf("\n");
    printf("cmp: "); for (size_t i=0; i<cmp_size; i++) printf("%02x ", (unsigned char)buf_cmp[i]); printf("\n");
    printf("mpk: "); for (size_t i=0; i<mpack_size; i++) printf("%02x ", (unsigned char)buf_mpack[i]); printf("\n");

    return 0;
}
