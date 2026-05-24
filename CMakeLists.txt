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
    include(CMakePackageConfigHelpers)

    install(TARGETS rmx EXPORT rmxTargets)
    install(DIRECTORY include/rmx DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
    install(
        EXPORT rmxTargets
        FILE rmxTargets.cmake
        NAMESPACE rmx::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/rmx
    )

    configure_package_config_file(
        cmake/rmxConfig.cmake.in ${CMAKE_CURRENT_BINARY_DIR}/rmxConfig.cmake
        INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/rmx
    )
    write_basic_package_version_file(
        ${CMAKE_CURRENT_BINARY_DIR}/rmxConfigVersion.cmake
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY SameMajorVersion
    )
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/rmxConfig.cmake
                  ${CMAKE_CURRENT_BINARY_DIR}/rmxConfigVersion.cmake
            DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/rmx
    )
endif()

if(PROJECT_IS_TOP_LEVEL)
    include(CTest)
    if(BUILD_TESTING)
        add_subdirectory(tests)
    endif()
endif()
