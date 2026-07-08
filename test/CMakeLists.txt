# add_subdirectory_once(lib/Radikant-Probe-C         Radikant-Probe-C)

# ---- Test Executables ----
add_executable(TST-MESSAGE-PACK     test_mp.c)
add_executable(TST-VECTOR           test_vector.c ../reference/cmp.c)
add_executable(GENERATE-VECTORS     vectors/generate_vectors.c ../reference/cmp.c)
add_executable(TST-PERF             test_perf.c ../reference/cmp.c)
add_executable(TST-FUZZ             test_fuzz.c ../reference/cmp.c)
add_executable(TST-ROUNDTRIP        test_roundtrip.c)
add_executable(TST-SAX              test_sax.c)
add_executable(TST-ADVERSARIAL      test_adversarial.c)
add_executable(TST-BUILDER          test_builder.c)
add_executable(TST-ZONE             test_zone.c)
add_executable(TST-STREAM           test_stream.c)
add_executable(TST-OBJECT           test_object.c)

target_compile_definitions(TST-VECTOR PRIVATE PROJECT_ROOT="${CMAKE_SOURCE_DIR}")
target_compile_definitions(GENERATE-VECTORS PRIVATE PROJECT_ROOT="${CMAKE_SOURCE_DIR}")

# ---- Link Executables ----
target_link_libraries(TST-MESSAGE-PACK PRIVATE Radikant-MessagePack-C Radikant-Probe-C)
target_link_libraries(TST-VECTOR       PRIVATE Radikant-MessagePack-C Radikant-Probe-C)
target_link_libraries(TST-PERF         PRIVATE Radikant-MessagePack-C Radikant-Probe-C mpack)
target_link_libraries(TST-FUZZ         PRIVATE Radikant-MessagePack-C Radikant-Probe-C)
target_link_libraries(TST-ROUNDTRIP    PRIVATE Radikant-MessagePack-C Radikant-Probe-C)
target_link_libraries(TST-SAX          PRIVATE Radikant-MessagePack-C Radikant-Probe-C)
target_link_libraries(TST-ADVERSARIAL  PRIVATE Radikant-MessagePack-C Radikant-Probe-C)
target_link_libraries(TST-BUILDER      PRIVATE Radikant-MessagePack-C Radikant-Probe-C)
target_link_libraries(TST-ZONE         PRIVATE Radikant-MessagePack-C Radikant-Probe-C)
target_link_libraries(TST-STREAM       PRIVATE Radikant-MessagePack-C Radikant-Probe-C)
target_link_libraries(TST-OBJECT       PRIVATE Radikant-MessagePack-C Radikant-Probe-C)

# Provide the project root so tests can load external files
target_compile_definitions(TST-PERF         PRIVATE PROJECT_ROOT="${CMAKE_SOURCE_DIR}")
target_compile_definitions(TST-FUZZ         PRIVATE PROJECT_ROOT="${CMAKE_SOURCE_DIR}")
target_compile_definitions(TST-ROUNDTRIP    PRIVATE PROJECT_ROOT="${CMAKE_SOURCE_DIR}")

# --- Cmake Test 
add_test(NAME TST-MESSAGE-PACK COMMAND TST-MESSAGE-PACK)
add_test(NAME TST-VECTOR       COMMAND TST-VECTOR)
#add_test(NAME TST-PERF         COMMAND TST-PERF)
add_test(NAME TST-FUZZ         COMMAND TST-FUZZ)
add_test(NAME TST-ROUNDTRIP    COMMAND TST-ROUNDTRIP)
add_test(NAME TST-SAX          COMMAND TST-SAX)
add_test(NAME TST-ADVERSARIAL  COMMAND TST-ADVERSARIAL)
add_test(NAME TST-BUILDER      COMMAND TST-BUILDER)
add_test(NAME TST-ZONE         COMMAND TST-ZONE)
add_test(NAME TST-STREAM       COMMAND TST-STREAM)
add_test(NAME TST-OBJECT       COMMAND TST-OBJECT)