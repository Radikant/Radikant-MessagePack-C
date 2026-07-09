// Std
#include <stdio.h>
#include <string.h>

// Radikant
#include <radikant-messagepack-c.h>
#include <radikant-probe-c.h>

bool msgpack_test_encode_decode(test_result_t *test);
bool msgpack_test_bin(test_result_t *test);
bool msgpack_test_ext(test_result_t *test);
bool msgpack_test_map(test_result_t *test);
bool msgpack_test_nil(test_result_t *test);
bool msgpack_test_type_casting(test_result_t *test);
test_suite_t suite_msgpack = {.name = "MessagePack Stream/API Suite",
                              .standard = "MsgPack-Protocol"};

int main(void) {
  r_set_global_verbosity(R_VERBOSE);
  enable_memleak_detection(&suite_msgpack);

  add_test(&suite_msgpack, msgpack_test_encode_decode,
           "Encode/Decode via Streams", "MsgPack-Spec");
  add_test(&suite_msgpack, msgpack_test_bin, "Encode/Decode Binary",
           "MsgPack-Spec");
  add_test(&suite_msgpack, msgpack_test_ext, "Encode/Decode Extension",
           "MsgPack-Spec");
  add_test(&suite_msgpack, msgpack_test_map, "Encode/Decode Map",
           "MsgPack-Spec");
  add_test(&suite_msgpack, msgpack_test_nil, "Encode/Decode Nil",
           "MsgPack-Spec");
  add_test(&suite_msgpack, msgpack_test_type_casting,
           "Safe Type Casting Safeguards", "MsgPack-Spec");
  register_suite(&suite_msgpack);
  bool success = run_all_suites();

  free_suite(&suite_msgpack);
  return success ? 0 : 1;
}

bool msgpack_test_encode_decode(test_result_t *test) {
  mp_stream_t write_stream;
  mp_stream_buffer_t write_buffer;
  mp_stream_init_write(&write_stream, &write_buffer, true, NULL, 0);

  // Encode
  mp_encode_array_len(&write_stream, 5);
  mp_encode_int(&write_stream, 1);
  mp_encode_str(&write_stream, "hello", 5);
  mp_encode_bool(&write_stream, true);
  mp_encode_int(&write_stream, -42);
  mp_encode_float(&write_stream, 3.14f);

  // Decode Top-Level API
  mp_zone_t zone;
  mp_zone_init(&zone, 4096);

  mp_object_t obj;
  mp_error_t err = mp_parse_memory(&zone, write_buffer.data, write_buffer.size, &obj);

  if (err != MP_OK) {
    append_error(test, "Decode failed with error code", err);
  } else {
    mp_object_t *elements;
    uint32_t len;

    if (mp_object_as_array(&obj, &elements, &len) != MP_OK) {
      append_error(test, "Expected ARRAY type", obj.type);
    } else if (len != 5) {
      append_error(test, "Expected array size 5", len);
    } else {
      int64_t i_val;
      if (mp_object_as_int(&elements[0], &i_val) != MP_OK || i_val != 1) {
        append_error(test, "Index 0: Expected integer 1", 0);
      }

      const char *s_val;
      uint32_t s_len;
      if (mp_object_as_str(&elements[1], &s_val, &s_len) != MP_OK ||
          s_len != 5 || strncmp(s_val, "hello", 5) != 0) {
        append_error(test, "Index 1: Expected string 'hello'", 0);
      }

      bool b_val;
      if (mp_object_as_bool(&elements[2], &b_val) != MP_OK || b_val != true) {
        append_error(test, "Index 2: Expected bool true", 0);
      }

      if (mp_object_as_int(&elements[3], &i_val) != MP_OK || i_val != -42) {
        append_error(test, "Index 3: Expected integer -42", 0);
      }

      float f_val;
      if (mp_object_as_float(&elements[4], &f_val) != MP_OK) {
        append_error(test, "Index 4: Expected FLOAT32", 0);
      }
    }
  }

  mp_zone_destroy(&zone);
  mp_memory_stream_destroy(&write_buffer);

  return test_end(test);
}

bool msgpack_test_bin(test_result_t *test) {
  mp_stream_t write_stream;
  mp_stream_buffer_t write_buffer;
  mp_stream_init_write(&write_stream, &write_buffer, true, NULL, 0);

  const char *bin_data = "\x00\x01\x02\xFF";
  mp_encode_bin(&write_stream, bin_data, 4);

  mp_zone_t zone;
  mp_zone_init(&zone, 4096);
  mp_object_t obj;
  mp_error_t err = mp_parse_memory(&zone, write_buffer.data, write_buffer.size, &obj);

  if (err != MP_OK) {
    append_error(test, "Decode failed", err);
  } else {
    const char *out_bin;
    uint32_t out_len;
    if (mp_object_as_bin(&obj, &out_bin, &out_len) != MP_OK) {
      append_error(test, "Expected BIN type", obj.type);
    } else if (out_len != 4 || memcmp(out_bin, bin_data, 4) != 0) {
      append_error(test, "Binary data mismatch", 0);
    }
  }

  mp_zone_destroy(&zone);
  mp_memory_stream_destroy(&write_buffer);
  return test_end(test);
}

bool msgpack_test_ext(test_result_t *test) {
  mp_stream_t write_stream;
  mp_stream_buffer_t write_buffer;
  mp_stream_init_write(&write_stream, &write_buffer, true, NULL, 0);

  const char *ext_data = "custom";
  mp_encode_ext(&write_stream, 42, ext_data, 6);

  mp_zone_t zone;
  mp_zone_init(&zone, 4096);
  mp_object_t obj;
  mp_error_t err = mp_parse_memory(&zone, write_buffer.data, write_buffer.size, &obj);

  if (err != MP_OK) {
    append_error(test, "Decode failed", err);
  } else {
    int8_t out_type;
    const char *out_data;
    uint32_t out_len;
    if (mp_object_as_ext(&obj, &out_type, &out_data, &out_len) != MP_OK) {
      append_error(test, "Expected EXT type", obj.type);
    } else if (out_type != 42 || out_len != 6 ||
               memcmp(out_data, ext_data, 6) != 0) {
      append_error(test, "Extension data/type mismatch", 0);
    }
  }

  mp_zone_destroy(&zone);
  mp_memory_stream_destroy(&write_buffer);
  return test_end(test);
}

bool msgpack_test_map(test_result_t *test) {
  mp_stream_t write_stream;
  mp_stream_buffer_t write_buffer;
  mp_stream_init_write(&write_stream, &write_buffer, true, NULL, 0);

  mp_encode_map_len(&write_stream, 1);
  mp_encode_str(&write_stream, "key", 3);
  mp_encode_int(&write_stream, 123);

  mp_zone_t zone;
  mp_zone_init(&zone, 4096);
  mp_object_t obj;
  mp_error_t err = mp_parse_memory(&zone, write_buffer.data, write_buffer.size, &obj);

  if (err != MP_OK) {
    append_error(test, "Decode failed", err);
  } else {
    mp_object_kv_t *elements;
    uint32_t len;
    if (mp_object_as_map(&obj, &elements, &len) != MP_OK) {
      append_error(test, "Expected MAP type", obj.type);
    } else if (len != 1) {
      append_error(test, "Expected map size 1", len);
    } else {
      const char *k_str;
      uint32_t k_len;
      if (mp_object_as_str(&elements[0].key, &k_str, &k_len) != MP_OK ||
          k_len != 3 || strncmp(k_str, "key", 3) != 0) {
        append_error(test, "Map key mismatch", 0);
      }
      int64_t v_int;
      if (mp_object_as_int(&elements[0].val, &v_int) != MP_OK || v_int != 123) {
        append_error(test, "Map value mismatch", 0);
      }
    }
  }

  mp_zone_destroy(&zone);
  mp_memory_stream_destroy(&write_buffer);
  return test_end(test);
}

bool msgpack_test_nil(test_result_t *test) {
  mp_stream_t write_stream;
  mp_stream_buffer_t write_buffer;
  mp_stream_init_write(&write_stream, &write_buffer, true, NULL, 0);

  mp_encode_nil(&write_stream);

  mp_zone_t zone;
  mp_zone_init(&zone, 4096);
  mp_object_t obj;
  mp_error_t err = mp_parse_memory(&zone, write_buffer.data, write_buffer.size, &obj);

  if (err != MP_OK) {
    append_error(test, "Decode failed", err);
  } else {
    if (mp_object_as_nil(&obj) != MP_OK) {
      append_error(test, "Expected NIL type", obj.type);
    }
  }

  mp_zone_destroy(&zone);
  mp_memory_stream_destroy(&write_buffer);
  return test_end(test);
}

bool msgpack_test_type_casting(test_result_t *test) {
  mp_object_t obj;
  obj.type = MP_TYPE_STR;
  obj.via.str.size = 5;
  obj.via.str.ptr = "hello";

  int64_t i_val;
  if (mp_object_as_int(&obj, &i_val) != MP_ERROR_OBJECT_TYPE_MISMATCH) {
    append_error(test, "String cast to Int did not safely fail", 0);
  }

  bool b_val;
  if (mp_object_as_bool(&obj, &b_val) != MP_ERROR_OBJECT_TYPE_MISMATCH) {
    append_error(test, "String cast to Bool did not safely fail", 0);
  }

  return test_end(test);
}