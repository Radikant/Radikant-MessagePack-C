# ---- Test Executables ----
add_executable(EXP-MAP          map.c)
add_executable(EXP-STREAMING    streaming.c)
add_executable(EXP-BASIC        basic.c)
add_executable(EXP-ARRAY        array.c)
add_executable(EXP-BUILDER      builder.c)
add_executable(EXP-NESTED       nested.c)

# ---- Link Executables ----
target_link_libraries(EXP-MAP            PRIVATE Radikant-MessagePack-C)
target_link_libraries(EXP-STREAMING      PRIVATE Radikant-MessagePack-C)
target_link_libraries(EXP-BASIC          PRIVATE Radikant-MessagePack-C)
target_link_libraries(EXP-ARRAY          PRIVATE Radikant-MessagePack-C)
target_link_libraries(EXP-BUILDER        PRIVATE Radikant-MessagePack-C)
target_link_libraries(EXP-NESTED         PRIVATE Radikant-MessagePack-C)