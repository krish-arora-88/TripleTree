# cmake/Toolchains/emscripten.cmake
# Placeholder toolchain for Emscripten (WebAssembly) builds.
# Point CMAKE_TOOLCHAIN_FILE at this file after installing the Emscripten SDK.
#
# Example:
#   cmake -S . -B build-wasm \
#         -DCMAKE_TOOLCHAIN_FILE=cmake/Toolchains/emscripten.cmake
#
# Requires EMSDK environment variable to point at the Emscripten SDK root.

if(DEFINED ENV{EMSDK})
    include("$ENV{EMSDK}/upstream/emscripten/cmake/Modules/Platform/Emscripten.cmake")
else()
    message(FATAL_ERROR
        "EMSDK environment variable not set.  "
        "Install Emscripten and source emsdk_env.sh first.")
endif()
