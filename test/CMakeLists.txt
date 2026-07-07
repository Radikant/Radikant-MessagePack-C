# add_subdirectory_once(lib/Radikant-Probe-C         Radikant-Probe-C)

# ---- Test Executables ----
add_executable(TST-MESSAGE-PACK     test_messagepack.c)
add_executable(TST-VECTOR           test_vector.c ../reference/cmp.c)
add_executable(GENERATE-VECTORS     generate_vectors.c ../reference/cmp.c)

target_compile_definitions(TST-VECTOR PRIVATE PROJECT_ROOT="${CMAKE_SOURCE_DIR}")
target_compile_definitions(GENERATE-VECTORS PRIVATE PROJECT_ROOT="${CMAKE_SOURCE_DIR}")

# ---- Link Executables ----
target_link_libraries(TST-MESSAGE-PACK      PRIVATE Radikant-MessagePack-C Radikant-Probe-C Radikant-Socket-C)
target_link_libraries(TST-VECTOR            PRIVATE Radikant-MessagePack-C Radikant-Probe-C Radikant-Socket-C)

# --- Cmake Test 
add_test(NAME TST-MESSAGE-PACK COMMAND TST-MESSAGE-PACK)
add_test(NAME TST-VECTOR COMMAND TST-VECTOR)
