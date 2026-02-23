# apps/wasm/CMakeLists.txt
# Builds the triplefill WASM module via Emscripten.
# Produces triplefill.js + triplefill.wasm in the binary dir.

add_executable(triplefill_wasm bindings.cpp)
target_link_libraries(triplefill_wasm PRIVATE triplefill)

set_target_properties(triplefill_wasm PROPERTIES
    OUTPUT_NAME triplefill
    SUFFIX      ".js"
)

# Emscripten link flags
target_link_options(triplefill_wasm PRIVATE
    "SHELL:-s MODULARIZE=1"
    "SHELL:-s EXPORT_NAME='TriplefillModule'"
    "SHELL:-s EXPORTED_FUNCTIONS=['_run_fill','_fill_create','_fill_frame_count','_fill_get_frame','_fill_destroy','_free_buffer','_malloc','_free']"
    "SHELL:-s EXPORTED_RUNTIME_METHODS=['ccall','cwrap','getValue','setValue','HEAPU8','HEAPU32','HEAPF64','wasmMemory']"
    "SHELL:-s INITIAL_MEMORY=33554432"
    "SHELL:-s ALLOW_MEMORY_GROWTH=1"
    "SHELL:-s MAXIMUM_MEMORY=1073741824"
    "SHELL:-s ENVIRONMENT='worker'"
    "SHELL:-s SINGLE_FILE=0"
    "SHELL:-s NO_EXIT_RUNTIME=1"
    -O3
    -fexceptions
)

# Need -fexceptions on the library too so try/catch works across TUs
target_compile_options(triplefill_wasm PRIVATE -fexceptions)
