#include <stdio.h>
#include <string.h>

// Radikant
#include <radikant-messagepack-c.h>
#include <radikant-probe-c.h>

bool msgpack_test_incomplete_read(test_result_t *test) {
  mp_zone_t zone;
  mp_zone_init(&zone, 4096);

  // Payload: map32 (0xdf) claiming to have 1,000,000 elements (0x00 0x0f 0x42
  // 0x40), but file ends
  const char payload[] = {(char)0xdf, 0x00, 0x0f, 0x42, 0x40};

  mp_object_t ast;
  mp_error_t err = mp_parse_memory(&zone, payload, sizeof(payload), &ast);

  if (err != MP_ERROR_DECODE_INCOMPLETE) {
    append_error(test, "Did not catch incomplete read properly", err);
  }

  mp_zone_destroy(&zone);
  return test_end(test);
}

bool msgpack_test_invalid_prefix(test_result_t *test) {
  mp_zone_t zone;
  mp_zone_init(&zone, 4096);

  // Payload: 0xC1 which is never used in MessagePack
  const char payload[] = {(char)0xc1};

  mp_object_t ast;
  mp_error_t err = mp_parse_memory(&zone, payload, sizeof(payload), &ast);

  if (err != MP_ERROR_DECODE_INVALID_FORMAT) {
    append_error(test, "Did not catch invalid prefix 0xC1", err);
  }

  mp_zone_destroy(&zone);
  return test_end(test);
}

bool msgpack_test_deep_recursion(test_result_t *test) {
  mp_zone_t zone;
  mp_zone_init(&zone, 4096);

  // Build a deeply nested array payload manually
  // 100,000 levels deep. Array of 1 element, which is array of 1 element, etc.
  size_t depth = 100000;
  char *payload = (char *)malloc(depth + 1);
  for (size_t i = 0; i < depth; i++) {
    payload[i] = (char)0x91; // fixarray with 1 element
  }
  payload[depth] = (char)0x00; // inner-most element is 0 (integer)

  mp_object_t ast;
  mp_error_t err = mp_parse_memory(&zone, payload, depth + 1, &ast);

  if (err != MP_ERROR_DECODE_DEPTH_EXCEEDED) {
    append_error(test, "Did not catch recursion depth limit", err);
  }

  free(payload);
  mp_zone_destroy(&zone);
  return test_end(test);
}

bool msgpack_test_giant_allocation(test_result_t *test) {
  mp_zone_t zone;
  mp_zone_init(&zone, 4096);

  // Payload: str32 claiming 2 GB size.
  const char payload[] = {(char)0xdb, 0x7f, (char)0xff, (char)0xff, (char)0xff};

  mp_object_t ast;
  mp_error_t err = mp_parse_memory(&zone, payload, sizeof(payload), &ast);

  // The parser should realize the string extends past the end of the payload
  // before allocating 2GB of memory.
  if (err != MP_ERROR_DECODE_TRUNCATED_STR) {
    printf("Giant allocation failed with error code: %d\n", err);
    append_error(test, "Did not protect against giant allocation length check",
                 err);
  }

  mp_zone_destroy(&zone);
  return test_end(test);
}

bool msgpack_test_giant_bin_allocation(test_result_t *test) {
  mp_zone_t zone;
  mp_zone_init(&zone, 4096);

  // Payload: bin32 claiming 2 GB size.
  const char payload[] = {(char)0xc6, 0x7f, (char)0xff, (char)0xff, (char)0xff};

  mp_object_t ast;
  mp_error_t err = mp_parse_memory(&zone, payload, sizeof(payload), &ast);

  if (err != MP_ERROR_DECODE_TRUNCATED_BIN) {
    append_error(test, "Did not protect against giant bin allocation length check", err);
  }

  mp_zone_destroy(&zone);
  return test_end(test);
}

bool msgpack_test_giant_ext_allocation(test_result_t *test) {
  mp_zone_t zone;
  mp_zone_init(&zone, 4096);

  // Payload: ext32 claiming 2 GB size.
  const char payload[] = {(char)0xc9, 0x7f, (char)0xff, (char)0xff, (char)0xff, 0x01};

  mp_object_t ast;
  mp_error_t err = mp_parse_memory(&zone, payload, sizeof(payload), &ast);

  if (err != MP_ERROR_DECODE_TRUNCATED_EXT) {
    append_error(test, "Did not protect against giant ext allocation length check", err);
  }

  mp_zone_destroy(&zone);
  return test_end(test);
}

bool msgpack_test_giant_array_allocation(test_result_t *test) {
  mp_zone_t zone;
  mp_zone_init(&zone, 4096);

  // Payload: array32 claiming 1 billion elements
  const char payload[] = {(char)0xdd, 0x3b, (char)0x9a, (char)0xca, 0x00};

  mp_object_t ast;
  mp_error_t err = mp_parse_memory(&zone, payload, sizeof(payload), &ast);

  // It should try to allocate, fail (or succeed and then fail decoding).
  // On most systems, 1 billion * sizeof(mp_object_t) (24 bytes) = 24 GB, which causes MP_ERROR_NOMEM.
  // If it succeeds, the read loop will instantly fail with MP_ERROR_DECODE_INCOMPLETE.
  if (err != MP_ERROR_NOMEM && err != MP_ERROR_DECODE_INCOMPLETE) {
    append_error(test, "Did not protect against giant array allocation", err);
  }

  mp_zone_destroy(&zone);
  return test_end(test);
}

bool msgpack_test_giant_map_allocation(test_result_t *test) {
  mp_zone_t zone;
  mp_zone_init(&zone, 4096);

  // Payload: map32 claiming 1 billion elements
  const char payload[] = {(char)0xdf, 0x3b, (char)0x9a, (char)0xca, 0x00};

  mp_object_t ast;
  mp_error_t err = mp_parse_memory(&zone, payload, sizeof(payload), &ast);

  if (err != MP_ERROR_NOMEM && err != MP_ERROR_DECODE_INCOMPLETE && err != MP_ERROR_DECODE_TRUNCATED_MAP) {
    append_error(test, "Did not protect against giant map allocation", err);
  }

  mp_zone_destroy(&zone);
  return test_end(test);
}

bool msgpack_test_deep_recursion_maps(test_result_t *test) {
  mp_zone_t zone;
  mp_zone_init(&zone, 4096);

  size_t depth = 1000; // Large depth
  char *payload = (char *)malloc(depth * 2 + 1);
  for (size_t i = 0; i < depth; i++) {
    payload[i*2] = (char)0x81;     // fixmap with 1 pair
    payload[i*2+1] = (char)0x00;   // key: int 0
  }
  payload[depth*2] = (char)0x00;   // value of innermost map: int 0

  mp_object_t ast;
  mp_error_t err = mp_parse_memory(&zone, payload, depth * 2 + 1, &ast);

  // If max_depth is 256, it should fail at depth 256.
  if (err != MP_ERROR_DECODE_DEPTH_EXCEEDED) {
    append_error(test, "Did not catch map recursion depth limit", err);
  }

  free(payload);
  mp_zone_destroy(&zone);
  return test_end(test);
}

test_suite_t suite_adversarial = {.name = "MessagePack Adversarial Suite",
                                  .standard = "MsgPack-Protocol"};

int main(void) {
  r_set_global_verbosity(R_VERBOSE);
  enable_memleak_detection(&suite_adversarial);

  add_test(&suite_adversarial, msgpack_test_incomplete_read,
           "Buffer Over-Read Safeguards", "MsgPack-Spec");
  add_test(&suite_adversarial, msgpack_test_invalid_prefix,
           "Reserved Prefix Safeguards", "MsgPack-Spec");
  add_test(&suite_adversarial, msgpack_test_deep_recursion,
           "Stack Exhaustion Safeguards", "MsgPack-Spec");
  add_test(&suite_adversarial, msgpack_test_giant_allocation,
           "Giant Allocation Safeguards", "MsgPack-Spec");
  add_test(&suite_adversarial, msgpack_test_giant_bin_allocation,
           "Giant Bin Allocation Safeguards", "MsgPack-Spec");
  add_test(&suite_adversarial, msgpack_test_giant_ext_allocation,
           "Giant Ext Allocation Safeguards", "MsgPack-Spec");
  add_test(&suite_adversarial, msgpack_test_giant_array_allocation,
           "Giant Array Allocation Safeguards", "MsgPack-Spec");
  add_test(&suite_adversarial, msgpack_test_giant_map_allocation,
           "Giant Map Allocation Safeguards", "MsgPack-Spec");
  add_test(&suite_adversarial, msgpack_test_deep_recursion_maps,
           "Map Stack Exhaustion Safeguards", "MsgPack-Spec");

  register_suite(&suite_adversarial);
  bool success = run_all_suites();
  return success ? 0 : 1;
}
