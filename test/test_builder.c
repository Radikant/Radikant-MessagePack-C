#include <stdio.h>
#include <string.h>

// Radikant
#include <radikant-messagepack-c.h>
#include <radikant-probe-c.h>

#define EXPECT_TRUE(cond, msg) \
  do { \
    if (!(cond)) { \
      append_error(test, msg, -1); \
    } \
  } while (0)

bool builder_test_primitives(test_result_t *test) {
  mp_object_t obj;

  // Primitives
  mp_build_nil(&obj);
  EXPECT_TRUE(obj.type == MP_TYPE_NIL, "Build NIL");

  mp_build_bool(&obj, true);
  EXPECT_TRUE(obj.type == MP_TYPE_BOOLEAN && obj.via.boolean == true,
              "Build BOOL (true)");

  mp_build_int(&obj, -42);
  EXPECT_TRUE(obj.type == MP_TYPE_NEGATIVE_INTEGER && obj.via.i64 == -42,
              "Build INT (-42)");

  mp_build_int(&obj, 42);
  EXPECT_TRUE(obj.type == MP_TYPE_POSITIVE_INTEGER && obj.via.u64 == 42,
              "Build INT (42)");

  mp_build_uint(&obj, 100);
  EXPECT_TRUE(obj.type == MP_TYPE_POSITIVE_INTEGER && obj.via.u64 == 100,
              "Build UINT (100)");

  mp_build_float(&obj, 3.14f);
  EXPECT_TRUE(obj.type == MP_TYPE_FLOAT32 && obj.via.f32 == 3.14f,
              "Build FLOAT (3.14)");

  mp_build_double(&obj, 3.14159);
  EXPECT_TRUE(obj.type == MP_TYPE_FLOAT64 && obj.via.f64 == 3.14159,
              "Build DOUBLE (3.14159)");

  mp_build_cstr(&obj, "hello");
  EXPECT_TRUE(obj.type == MP_TYPE_STR && obj.via.str.size == 5 &&
                  strncmp(obj.via.str.ptr, "hello", 5) == 0,
              "Build STR (hello)");

  mp_build_bin(&obj, "data", 4);
  EXPECT_TRUE(obj.type == MP_TYPE_BIN && obj.via.bin.size == 4 &&
                  strncmp(obj.via.bin.ptr, "data", 4) == 0,
              "Build BIN (data)");

  mp_build_ext(&obj, 1, "ext", 3);
  EXPECT_TRUE(obj.type == MP_TYPE_EXT && obj.via.ext.type == 1 &&
                  obj.via.ext.size == 3 &&
                  strncmp(obj.via.ext.ptr, "ext", 3) == 0,
              "Build EXT");

  return test_end(test);
}

bool builder_test_collections(test_result_t *test) {
  // Collections
  mp_zone_t zone;
  mp_zone_init(&zone, 1024);

  mp_object_t array;
  mp_build_array(&zone, &array, 2);
  EXPECT_TRUE(array.type == MP_TYPE_ARRAY && array.via.array.size == 2,
              "Build ARRAY (size 2)");

  mp_object_t v;
  mp_build_int(&v, 123);
  mp_array_set(&array, 0, v);
  mp_build_cstr(&v, "test");
  mp_array_set(&array, 1, v);

  EXPECT_TRUE(array.via.array.ptr[0].type == MP_TYPE_POSITIVE_INTEGER &&
                  array.via.array.ptr[0].via.u64 == 123,
              "Array Set [0] (123)");
  EXPECT_TRUE(array.via.array.ptr[1].type == MP_TYPE_STR &&
                  array.via.array.ptr[1].via.str.size == 4,
              "Array Set [1] (test)");

  mp_object_t map;
  mp_build_map(&zone, &map, 2);
  EXPECT_TRUE(map.type == MP_TYPE_MAP && map.via.map.size == 2,
              "Build MAP (size 2)");

  mp_map_set_str(&map, 0, "key1", "val1");
  mp_map_set_int(&map, 1, "key2", 456);

  EXPECT_TRUE(map.via.map.ptr[0].key.type == MP_TYPE_STR &&
                  map.via.map.ptr[0].val.type == MP_TYPE_STR,
              "Map Set [0] (cstr:cstr)");
  EXPECT_TRUE(map.via.map.ptr[1].key.type == MP_TYPE_STR &&
                  map.via.map.ptr[1].val.type == MP_TYPE_POSITIVE_INTEGER &&
                  map.via.map.ptr[1].val.via.u64 == 456,
              "Map Set [1] (cstr:int)");

  mp_zone_destroy(&zone);
  return test_end(test);
}

test_suite_t suite_builder = {.name = "MessagePack Builder API Suite",
                              .standard = "MsgPack-Spec"};

int main(void) {
  r_set_global_verbosity(R_VERBOSE);
  enable_memleak_detection(&suite_builder);

  add_test(&suite_builder, builder_test_primitives, "Builder API: Primitives",
           "MsgPack-Spec");
  add_test(&suite_builder, builder_test_collections, "Builder API: Collections",
           "MsgPack-Spec");

  bool success = run_single_suite(&suite_builder);
  free_suite(&suite_builder);
  return success ? 0 : 1;
}
