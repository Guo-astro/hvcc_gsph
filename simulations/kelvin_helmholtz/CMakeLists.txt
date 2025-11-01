cmake_minimum_required(VERSION 3.15)
project(kelvin_helmholtz_simulation)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Dimension parameter (must match main build)
if(NOT DEFINED DIM)
    set(DIM 2 CACHE STRING "Spatial dimension (1, 2, or 3)")
endif()

add_definitions(-DDIM=${DIM})

# Find the main SPH library
set(SPH_MAIN_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../..")
set(SPH_BUILD_DIR "${SPH_MAIN_DIR}/build")

# Include directories from main project
include_directories(
    ${SPH_MAIN_DIR}/include
    ${SPH_MAIN_DIR}/include/core
    ${SPH_MAIN_DIR}/include/utilities
    ${SPH_MAIN_DIR}/include/modules
    ${SPH_MAIN_DIR}/include/tree
    ${SPH_MAIN_DIR}/include/algorithms
    ${SPH_MAIN_DIR}/include/kernel
    ${SPH_MAIN_DIR}/include/samples
)

# Create shared library (plugin)
add_library(kelvin_helmholtz_plugin SHARED
    kelvin_helmholtz.cpp
)

# Link against main SPH library (if it exists)
if(EXISTS ${SPH_BUILD_DIR}/lib/libsph_lib.a)
    target_link_libraries(kelvin_helmholtz_plugin
        ${SPH_BUILD_DIR}/lib/libsph_lib.a
    )
else()
    message(WARNING "Main SPH library not found at ${SPH_BUILD_DIR}/lib/libsph_lib.a")
    message(WARNING "Please build the main project first: cd ${SPH_BUILD_DIR} && cmake .. && make")
endif()

# Set output directory
set_target_properties(kelvin_helmholtz_plugin PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
)

# Platform-specific settings
if(APPLE)
    set_target_properties(kelvin_helmholtz_plugin PROPERTIES
        SUFFIX ".dylib"
    )
endif()

message(STATUS "Building Kelvin-Helmholtz simulation plugin (DIM=${DIM})")
message(STATUS "  SPH library: ${SPH_BUILD_DIR}/lib/libsph_lib.a")
