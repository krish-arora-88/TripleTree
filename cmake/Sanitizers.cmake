# cmake/Sanitizers.cmake
# Provides a helper function to add AddressSanitizer + UndefinedBehaviorSanitizer
# compile/link flags to a target.  Only applies on compilers that support them.

function(triplefill_add_sanitizers target)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang|AppleClang")
        set(_flags -fsanitize=address,undefined -fno-omit-frame-pointer)
        target_compile_options(${target} PRIVATE ${_flags})
        target_link_options(${target}    PRIVATE ${_flags})
    elseif(MSVC)
        # MSVC supports /fsanitize=address (UBSan not available)
        target_compile_options(${target} PRIVATE /fsanitize=address)
    endif()
endfunction()
