cmake_minimum_required(VERSION 3.16)

project(
    rmx
    VERSION 0.1.0
    LANGUAGES CXX
)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

include(GNUInstallDirs)

# A CMake 3.16 version of https://cmake.org/cmake/help/latest/variable/PROJECT_IS_TOP_LEVEL.html
set(RMX_IS_MAIN_PROJECT OFF)
if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    message(STATUS "rmx is the top-level project")
    set(RMX_IS_MAIN_PROJECT ON)
endif()

option(RMX_BUILD_TESTS "Enable building the project tests" ${RMX_IS_MAIN_PROJECT})
# TODO: Maybe longer-running fuzz tests?

add_library(rmx INTERFACE)
target_include_directories(
    rmx INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
                  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_compile_features(rmx INTERFACE cxx_std_17)
install(DIRECTORY include/rmx DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

if(RMX_BUILD_TESTS)
    message(STATUS "Building rmx tests")
    enable_testing()
    add_subdirectory(tests)
endif()
