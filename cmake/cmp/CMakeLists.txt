add_library(cmp STATIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../../lib/cmp/cmp.c
)
target_include_directories(cmp PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/../../lib/cmp
)
