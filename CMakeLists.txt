cmake_minimum_required(VERSION 3.20)
project(Radikant-MessagePack-C VERSION 0.1.0 LANGUAGES C)

# ---- Include Radikant CMake functions ----
include("cmake/Radikant.cmake")

# ---- Detect Apple Silicon ----
detect_apple_silicon()

# ---- Submodules ----
add_subdirectory_once(lib/Radikant-Socket-C    Radikant-Socket-C)
add_subdirectory_once(lib/Radikant-Log-C       Radikant-Log-C)
add_subdirectory_once(lib/Radikant-Probe-C     Radikant-Probe-C)

# ---- Global settings ----
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ---- Sanitizers for Debug Builds ----
if(CMAKE_BUILD_TYPE MATCHES Debug)
    add_compile_options("-fsanitize=address,undefined" "-fno-omit-frame-pointer" "-g")
    add_link_options("-fsanitize=address,undefined")
endif()

# ---- Build shared library ----
add_library(Radikant-MessagePack-C SHARED
    SRC/encoder.c
    SRC/decoder.c
    SRC/types.c
    SRC/skip.c
    SRC/zone.c
    SRC/object.c
    SRC/error.c
    SRC/stream.c
    SRC/builder.c
)

# ---- Includess ----
target_include_directories(Radikant-MessagePack-C
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include> 
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/SRC>
        $<INSTALL_INTERFACE:include>
)

# ---- Link dependencies ----
target_link_libraries(Radikant-MessagePack-C
    PUBLIC
        # OpenSSL::SSL
        # OpenSSL::Crypto
        # Radikant-Crypto
    PRIVATE
        Radikant-Socket-C
        Radikant-Probe-C
        Radikant-Log-C
)

# ---- Output settings ----
set_target_properties(Radikant-MessagePack-C PROPERTIES
    OUTPUT_NAME "radikant_messagepack_c"
    VERSION ${PROJECT_VERSION}
    SOVERSION 1
)


if(NOT PROJECT_IS_TOP_LEVEL)
    return() 
endif()

add_subdirectory_once(lib/Radikant-Probe-C Radikant-Probe-C)
add_subdirectory_once(lib/mpack mpack)

enable_testing()
add_subdirectory(test)
add_subdirectory(example)