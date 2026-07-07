#ifndef RADIKANT_MESSAGEPACK_TYPES_H
#define RADIKANT_MESSAGEPACK_TYPES_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    MP_TYPE_NIL = 0,
    MP_TYPE_BOOLEAN,
    MP_TYPE_POSITIVE_INTEGER,
    MP_TYPE_NEGATIVE_INTEGER,
    MP_TYPE_FLOAT32,
    MP_TYPE_FLOAT64,
    MP_TYPE_STR,
    MP_TYPE_BIN,
    MP_TYPE_ARRAY,
    MP_TYPE_MAP,
    MP_TYPE_EXT
} mp_type_t;

typedef enum {
    MP_TAG_NIL = 0xc0,
    MP_TAG_UNUSED = 0xc1,
    MP_TAG_FALSE = 0xc2,
    MP_TAG_TRUE = 0xc3,
    MP_TAG_BIN8 = 0xc4,
    MP_TAG_BIN16 = 0xc5,
    MP_TAG_BIN32 = 0xc6,
    MP_TAG_EXT8 = 0xc7,
    MP_TAG_EXT16 = 0xc8,
    MP_TAG_EXT32 = 0xc9,
    MP_TAG_FLOAT32 = 0xca,
    MP_TAG_FLOAT64 = 0xcb,
    MP_TAG_UINT8 = 0xcc,
    MP_TAG_UINT16 = 0xcd,
    MP_TAG_UINT32 = 0xce,
    MP_TAG_UINT64 = 0xcf,
    MP_TAG_INT8 = 0xd0,
    MP_TAG_INT16 = 0xd1,
    MP_TAG_INT32 = 0xd2,
    MP_TAG_INT64 = 0xd3,
    MP_TAG_FIXEXT1 = 0xd4,
    MP_TAG_FIXEXT2 = 0xd5,
    MP_TAG_FIXEXT4 = 0xd6,
    MP_TAG_FIXEXT8 = 0xd7,
    MP_TAG_FIXEXT16 = 0xd8,
    MP_TAG_STR8 = 0xd9,
    MP_TAG_STR16 = 0xda,
    MP_TAG_STR32 = 0xdb,
    MP_TAG_ARRAY16 = 0xdc,
    MP_TAG_ARRAY32 = 0xdd,
    MP_TAG_MAP16 = 0xde,
    MP_TAG_MAP32 = 0xdf
} mp_tag_t;

#define MP_TAG_FIXINT_MAX 0x7f
#define MP_TAG_FIXMAP_MIN 0x80
#define MP_TAG_FIXMAP_MAX 0x8f
#define MP_TAG_FIXARRAY_MIN 0x90
#define MP_TAG_FIXARRAY_MAX 0x9f
#define MP_TAG_FIXSTR_MIN 0xa0
#define MP_TAG_FIXSTR_MAX 0xbf
#define MP_TAG_NEGFIXINT_MIN 0xe0
#define MP_TAG_NEGFIXINT_MAX 0xff

#ifdef __cplusplus
}
#endif

#endif // RADIKANT_MESSAGEPACK_TYPES_H
