# ---- Test Executables ----
add_executable(EXP-MESSAGEPACK  example_messagepack.c)

# ---- Link Executables ----
target_link_libraries(EXP-MESSAGEPACK    PRIVATE Radikant-MessagePack-C)
