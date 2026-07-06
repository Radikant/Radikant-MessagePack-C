# add_subdirectory_once(lib/Radikant-Probe-C         Radikant-Probe-C)

# ---- Test Executables ----
add_executable(TST-MESSAGE-PACK     test_messagepack.c)

# ---- Link Executables ----
target_link_libraries(TST-MESSAGE-PACK      PRIVATE Radikant-MessagePack-C Radikant-Probe-C Radikant-Socket-C)

# --- Cmake Test 
add_test(NAME TST-MESSAGE-PACK COMMAND TST-MESSAGE-PACK)
