add_library(mpack STATIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../../lib/mpack/src/mpack/mpack-common.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../lib/mpack/src/mpack/mpack-expect.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../lib/mpack/src/mpack/mpack-node.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../lib/mpack/src/mpack/mpack-platform.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../lib/mpack/src/mpack/mpack-reader.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../lib/mpack/src/mpack/mpack-writer.c
)
target_include_directories(mpack PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/../../lib/mpack/src 
    ${CMAKE_CURRENT_SOURCE_DIR}/../../lib/mpack/src/mpack
)
target_compile_definitions(mpack PUBLIC MPACK_EXTENSIONS=1)
