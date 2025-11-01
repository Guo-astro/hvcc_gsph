cmake_minimum_required(VERSION 3.23)
project(disk_relaxation_plugin)

# Set C++ standard
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find the main SPH library
get_filename_component(SPH_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../../../" ABSOLUTE)
set(SPH_BUILD_DIR "${SPH_ROOT}/build")
set(SPH_INCLUDE_DIR "${SPH_ROOT}/include")

# Include directories
include_directories(${SPH_INCLUDE_DIR})

# Add compile definitions for DIM=3
add_compile_definitions(DIM=3)

# Create shared library (plugin)
add_library(disk_relaxation_plugin SHARED
    disk_relaxation.cpp
)

# Link against the main SPH library
target_link_libraries(disk_relaxation_plugin
    PRIVATE ${SPH_BUILD_DIR}/libsph_lib.a
)

# Set output directory
set_target_properties(disk_relaxation_plugin PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/build"
    PREFIX "lib"
    SUFFIX ".dylib"
)

# macOS specific settings
if(APPLE)
    set_target_properties(disk_relaxation_plugin PROPERTIES
        MACOSX_RPATH ON
        BUILD_WITH_INSTALL_RPATH ON
        INSTALL_NAME_DIR "@rpath"
    )
endif()

message(STATUS "Disk Relaxation Plugin configuration:")
message(STATUS "  SPH library: ${SPH_BUILD_DIR}/libsph_lib.a")
message(STATUS "  Include dir: ${SPH_INCLUDE_DIR}")
message(STATUS "  Output: ${CMAKE_CURRENT_SOURCE_DIR}/build/libdisk_relaxation_plugin.dylib")
