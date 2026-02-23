cmake_minimum_required(VERSION 3.20)
project(triplefill
    VERSION   1.0.0
    LANGUAGES CXX
    DESCRIPTION "Flood-fill animation library and CLI"
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ---- options ---------------------------------------------------------------
option(TRIPLEFILL_BUILD_TESTS "Build unit tests"     ON)
option(TRIPLEFILL_BUILD_CLI   "Build CLI application" ON)
option(TRIPLEFILL_BUILD_WASM  "Build WASM bindings"   OFF)
option(TRIPLEFILL_SANITIZERS  "Enable ASan + UBSan"   OFF)

include(cmake/Sanitizers.cmake)

# ---- third-party: lodepng (compiled as object library) ---------------------
add_library(lodepng OBJECT third_party/lodepng/lodepng.cpp)
target_include_directories(lodepng PUBLIC third_party/lodepng)
# lodepng triggers some warnings under strict settings — suppress them
target_compile_options(lodepng PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-w>
    $<$<CXX_COMPILER_ID:MSVC>:/w>)

# ---- core library ----------------------------------------------------------
add_library(triplefill
    src/image_png.cpp
    src/tolerance.cpp
    src/fill.cpp
    src/animation.cpp
    src/pickers/solid.cpp
    src/pickers/stripe.cpp
    src/pickers/quarter.cpp
    src/pickers/border.cpp
)

target_include_directories(triplefill
    PUBLIC  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/third_party/lodepng
            ${CMAKE_CURRENT_SOURCE_DIR}/third_party/gif
)
target_link_libraries(triplefill PRIVATE lodepng)

# Strict warnings for our own code
target_compile_options(triplefill PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-Wall -Wextra -Wpedantic>
    $<$<CXX_COMPILER_ID:MSVC>:/W4>)

if(TRIPLEFILL_BUILD_WASM)
    target_compile_options(triplefill PRIVATE -fexceptions)
    target_compile_options(lodepng PRIVATE -fexceptions)
endif()

if(TRIPLEFILL_SANITIZERS)
    triplefill_add_sanitizers(triplefill)
endif()

# ---- CLI application -------------------------------------------------------
if(TRIPLEFILL_BUILD_CLI)
    add_subdirectory(apps/cli)
endif()

# ---- WASM bindings ---------------------------------------------------------
if(TRIPLEFILL_BUILD_WASM)
    add_subdirectory(apps/wasm)
endif()

# ---- tests -----------------------------------------------------------------
if(TRIPLEFILL_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
