#include "print.h"
#include <stdio.h>
#include <ctype.h>

static void print_indent(int indent) {
    for (int i = 0; i < indent; i++) {
        printf("  ");
    }
}

void mp_print_object_indented(const mp_object_t* obj, int indent) {
    if (!obj) {
        printf("null");
        return;
    }

    switch (obj->type) {
        case MP_TYPE_NIL:
            printf("null");
            break;
        case MP_TYPE_BOOLEAN:
            printf("%s", obj->via.boolean ? "true" : "false");
            break;
        case MP_TYPE_POSITIVE_INTEGER:
            printf("%llu", (unsigned long long)obj->via.u64);
            break;
        case MP_TYPE_NEGATIVE_INTEGER:
            printf("%lld", (long long)obj->via.i64);
            break;
        case MP_TYPE_FLOAT32:
            printf("%f", obj->via.f32);
            break;
        case MP_TYPE_FLOAT64:
            printf("%f", obj->via.f64);
            break;
        case MP_TYPE_STR:
            printf("\"%.*s\"", (int)obj->via.str.size, obj->via.str.ptr);
            break;
        case MP_TYPE_BIN:
            printf("<Binary Data: %u bytes>", obj->via.bin.size);
            break;
        case MP_TYPE_EXT:
            printf("<Extension Data: type=%d, %u bytes>", obj->via.ext.type, obj->via.ext.size);
            break;
        case MP_TYPE_ARRAY:
            if (obj->via.array.size == 0) {
                printf("[]");
            } else {
                printf("[\n");
                for (uint32_t i = 0; i < obj->via.array.size; i++) {
                    print_indent(indent + 1);
                    mp_print_object_indented(&obj->via.array.ptr[i], indent + 1);
                    if (i < obj->via.array.size - 1) {
                        printf(",");
                    }
                    printf("\n");
                }
                print_indent(indent);
                printf("]");
            }
            break;
        case MP_TYPE_MAP:
            if (obj->via.map.size == 0) {
                printf("{}");
            } else {
                printf("{\n");
                for (uint32_t i = 0; i < obj->via.map.size; i++) {
                    print_indent(indent + 1);
                    // Print key (usually string)
                    mp_print_object_indented(&obj->via.map.ptr[i].key, 0);
                    printf(": ");
                    // Print value
                    mp_print_object_indented(&obj->via.map.ptr[i].val, indent + 1);
                    if (i < obj->via.map.size - 1) {
                        printf(",");
                    }
                    printf("\n");
                }
                print_indent(indent);
                printf("}");
            }
            break;
        default:
            printf("<Unknown Type>");
            break;
    }
}

void mp_print_object(const mp_object_t* obj) {
    mp_print_object_indented(obj, 0);
    printf("\n");
}

void mp_print_hex_dump(const char* data, size_t size) {
    const unsigned char* udata = (const unsigned char*)data;
    size_t i, j;

    for (i = 0; i < size; i += 16) {
        // Print offset
        printf("%08zx  ", i);

        // Print hex bytes
        for (j = 0; j < 16; j++) {
            if (i + j < size) {
                printf("%02x ", udata[i + j]);
            } else {
                printf("   ");
            }
            if (j == 7) printf(" "); // Extra space after 8 bytes
        }

        printf(" |");

        // Print ASCII chars
        for (j = 0; j < 16; j++) {
            if (i + j < size) {
                unsigned char c = udata[i + j];
                printf("%c", isprint(c) ? c : '.');
            }
        }
        printf("|\n");
    }
}
