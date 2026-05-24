add_library(rmx-sanitizers INTERFACE)

set(_RMX_SANITIZER_FLAGS)
if(RMX_ENABLE_ASAN)
    list(APPEND _RMX_SANITIZER_FLAGS -fsanitize=address)
endif()
if(RMX_ENABLE_TSAN)
    list(APPEND _RMX_SANITIZER_FLAGS -fsanitize=thread)
endif()

# UBSAN can be enabled together with ASAN or TSAN, but ASAN and TSAN cannot be enabled together
if(RMX_ENABLE_ASAN OR RMX_ENABLE_TSAN)
    list(
        APPEND
        _RMX_SANITIZER_FLAGS
        -fsanitize=undefined
        -fno-omit-frame-pointer
    )
endif()
if(RMX_ENABLE_ASAN AND RMX_ENABLE_TSAN)
    message(FATAL_ERROR "RMX_ENABLE_ASAN and RMX_ENABLE_TSAN are mutually exclusive")
endif()

if(_RMX_SANITIZER_FLAGS AND CMAKE_CXX_COMPILER_ID MATCHES "^(GNU|Clang|AppleClang)$")
    target_compile_options(rmx-sanitizers INTERFACE ${_RMX_SANITIZER_FLAGS})
    target_link_options(rmx-sanitizers INTERFACE ${_RMX_SANITIZER_FLAGS})
endif()
