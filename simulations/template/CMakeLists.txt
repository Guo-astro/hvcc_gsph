cmake_minimum_required(VERSION 3.15)
project(template_simulation)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find the main SPH library
# Assumes this is built from simulations/template/build
# and main library is at ../../build
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
)

# Create shared library (plugin)
add_library(template_simulation_plugin SHARED
    template_simulation.cpp
)

# Link against main SPH library
target_link_libraries(template_simulation_plugin
    ${SPH_BUILD_DIR}/lib/libsph_lib.a
)

# Set output directory
set_target_properties(template_simulation_plugin PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    PREFIX ""  # Don't add 'lib' prefix (optional)
)

# Platform-specific settings
if(APPLE)
    set_target_properties(template_simulation_plugin PROPERTIES
        SUFFIX ".dylib"
    )
endif()

message(STATUS "Building template simulation plugin")
message(STATUS "  Output: ${CMAKE_CURRENT_BINARY_DIR}/template_simulation_plugin")
