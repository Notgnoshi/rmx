cmake_minimum_required(VERSION 3.28)

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

option(RMX_ENABLE_WERROR "Treat warnings as errors when building rmx's own tests" ON)
# Prevent including rmx in consuming project's installations.
option(RMX_ENABLE_INSTALL "Generate install rules for rmx" ${PROJECT_IS_TOP_LEVEL})
option(RMX_ENABLE_TSAN "Enable Thread Sanitizer and UBSan when building rmx's own tests" ON)
option(RMX_ENABLE_ASAN "Enable Address Sanitizer and UBSan when building rmx's own tests" OFF)

add_library(rmx INTERFACE)
add_library(rmx::rmx ALIAS rmx)
target_include_directories(
    rmx INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
                  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_compile_features(rmx INTERFACE cxx_std_17)

if(RMX_ENABLE_INSTALL)
    install(DIRECTORY include/rmx DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
endif()

if(PROJECT_IS_TOP_LEVEL)
    include(CTest)
    if(BUILD_TESTING)
        add_subdirectory(tests)
    endif()
endif()
