#ifndef RADIKANT_MESSAGEPACK_SKIP_H
#define RADIKANT_MESSAGEPACK_SKIP_H

#ifdef __cplusplus
extern "C" {
#endif

typedef enum // unsure if the error is a architectually a sound move.
{ SKIP_OK,
  SKIP_FAIL,
} skip_error_t;

#ifdef __cplusplus
}
#endif

#endif // RADIKANT_SKIP_H
