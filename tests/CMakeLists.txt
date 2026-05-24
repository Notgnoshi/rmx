find_package(Catch2 3 REQUIRED)
include(Catch)
include(RmxCompilerWarnings)

add_executable(rmx-tests)

file(
    GLOB_RECURSE
    RMX_TEST_SOURCES
    CONFIGURE_DEPENDS
    *.cpp
)
target_sources(rmx-tests PRIVATE "${RMX_TEST_SOURCES}")

target_link_libraries(rmx-tests PRIVATE rmx Catch2::Catch2WithMain rmx-compiler-warnings)
if(RMX_ENABLE_WERROR)
    target_link_libraries(rmx-tests PRIVATE rmx-werror)
endif()
catch_discover_tests(rmx-tests)
