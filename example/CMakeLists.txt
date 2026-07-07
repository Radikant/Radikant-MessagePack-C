# ---- Test Executables ----
add_executable(EXP-MESSAGEPACK  example_messagepack.c)
add_executable(EXP-SIMPLE       example_mp_simple.c)

# ---- Link Executables ----
target_link_libraries(EXP-MESSAGEPACK    PRIVATE Radikant-MessagePack-C)
target_link_libraries(EXP-SIMPLE         PRIVATE Radikant-MessagePack-C)
