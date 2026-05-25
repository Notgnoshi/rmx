add_library(rmx-compiler-warnings INTERFACE)
add_library(rmx-werror INTERFACE)

set(_RMX_GCC_CLANG_WARNINGS
    -Wall
    -Wextra
    -Wpedantic
    -Wshadow
    -Wnon-virtual-dtor
    -Wold-style-cast
    -Wcast-align
    -Wcast-qual
    -Wunused
    -Woverloaded-virtual
    -Wconversion
    -Wsign-conversion
    -Wmisleading-indentation
    -Wnull-dereference
    -Wdouble-promotion
    -Wformat=2
    -Wimplicit-fallthrough
    -Wundef
    -Wzero-as-null-pointer-constant
    -Wextra-semi
    -Wsuggest-override
    -Wmissing-include-dirs
)

set(_RMX_GCC_ONLY_WARNINGS
    -Wduplicated-cond
    -Wduplicated-branches
    -Wlogical-op
    -Wuseless-cast
    -Wstrict-null-sentinel
    -Wstrict-aliasing=3
    -Wformat-overflow=2
    -Wformat-truncation=2
    -Wstringop-overflow=4
)

set(_RMX_MSVC_WARNINGS /W4 /permissive-)

if(MSVC)
    target_compile_options(rmx-compiler-warnings INTERFACE ${_RMX_MSVC_WARNINGS})
    target_compile_options(rmx-werror INTERFACE /WX)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "^(GNU|Clang|AppleClang)$")
    target_compile_options(rmx-compiler-warnings INTERFACE ${_RMX_GCC_CLANG_WARNINGS})
    target_compile_options(rmx-werror INTERFACE -Werror)
endif()

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(rmx-compiler-warnings INTERFACE ${_RMX_GCC_ONLY_WARNINGS})
endif()
