find_package(Catch2 3 REQUIRED)
include(Catch)
include(RmxCompilerWarnings)
include(RmxSanitizers)

add_executable(rmx-tests)

file(
    GLOB_RECURSE
    RMX_TEST_SOURCES
    CONFIGURE_DEPENDS
    *.cpp
)
target_sources(rmx-tests PRIVATE "${RMX_TEST_SOURCES}")

target_link_libraries(
    rmx-tests
    PRIVATE rmx::rmx Catch2::Catch2WithMain rmx-compiler-warnings rmx-sanitizers
)
if(RMX_ENABLE_WERROR)
    target_link_libraries(rmx-tests PRIVATE rmx-werror)
endif()
catch_discover_tests(rmx-tests)
