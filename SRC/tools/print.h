#ifndef RADIKANT_MESSAGEPACK_C_PRINT_H
#define RADIKANT_MESSAGEPACK_C_PRINT_H

#include "../object.h"
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// -----------------------------------------------------------------------------
// AST Printing (Decoded Data)
// -----------------------------------------------------------------------------

// Prints an AST object recursively formatted like JSON
void mp_print_object(const mp_object_t* obj);

// Helper for indented printing
void mp_print_object_indented(const mp_object_t* obj, int indent);

// -----------------------------------------------------------------------------
// Binary Printing (Encoded Data)
// -----------------------------------------------------------------------------

// Prints raw binary data as a classic 16-byte hex dump
void mp_print_hex_dump(const char* data, size_t size);

#ifdef __cplusplus
}
#endif

#endif // RADIKANT_MESSAGEPACK_C_PRINT_H
