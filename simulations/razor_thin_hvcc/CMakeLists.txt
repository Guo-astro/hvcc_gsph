cmake_minimum_required(VERSION 3.15)
project(razor_thin_hvcc_simulation)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Dimension parameter (must be 3 for this simulation)
if(NOT DEFINED DIM)
    set(DIM 3 CACHE STRING "Spatial dimension (must be 3)")
endif()

if(NOT DIM EQUAL 3)
    message(FATAL_ERROR "razor_thin_hvcc requires DIM=3 (uses 2.5D mode)")
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
add_library(razor_thin_hvcc_plugin SHARED
    razor_thin_hvcc.cpp
)

# Link against main SPH library (if it exists)
if(EXISTS ${SPH_BUILD_DIR}/lib/libsph_lib.a)
    target_link_libraries(razor_thin_hvcc_plugin
        ${SPH_BUILD_DIR}/lib/libsph_lib.a
    )
else()
    message(WARNING "Main SPH library not found at ${SPH_BUILD_DIR}/lib/libsph_lib.a")
    message(WARNING "Please build the main project first: cd ${SPH_BUILD_DIR} && cmake .. && make")
endif()

# Set output directory
set_target_properties(razor_thin_hvcc_plugin PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
)

# Platform-specific settings
if(APPLE)
    set_target_properties(razor_thin_hvcc_plugin PROPERTIES
        SUFFIX ".dylib"
    )
endif()

message(STATUS "Building Razor-thin HVCC simulation plugin (DIM=${DIM})")
message(STATUS "  SPH library: ${SPH_BUILD_DIR}/lib/libsph_lib.a")
