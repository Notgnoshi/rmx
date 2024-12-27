find_package(Catch2 3 REQUIRED)

add_executable(rmx-tests)

file(
    GLOB_RECURSE
    RMX_TEST_SOURCES
    CONFIGURE_DEPENDS
    *.cpp
)
target_sources(rmx-tests PRIVATE "${RMX_TEST_SOURCES}")

target_link_libraries(rmx-tests PRIVATE rmx Catch2::Catch2WithMain)
