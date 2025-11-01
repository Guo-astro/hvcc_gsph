cmake_minimum_required(VERSION 3.10)

# Include parent CMake modules
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/../../cmake")

# Set build dimension
set(BUILD_DIM 2 CACHE STRING "Build dimension (1, 2, or 3)")

# Get parent project includes
set(SPH_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../include")

# Define the plugin
add_library(sedov_taylor_2d_gdisph_plugin SHARED sedov_taylor_2d.cpp)

# Set dimension compile definition
target_compile_definitions(sedov_taylor_2d_gdisph_plugin PRIVATE DIM=${BUILD_DIM})

# Include directories
target_include_directories(sedov_taylor_2d_gdisph_plugin PRIVATE ${SPH_INCLUDE_DIR})

# C++ standard
set_target_properties(sedov_taylor_2d_gdisph_plugin PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    POSITION_INDEPENDENT_CODE ON
)

# Platform-specific settings
if(APPLE)
    set_target_properties(sedov_taylor_2d_gdisph_plugin PROPERTIES
        SUFFIX ".dylib"
    )
elseif(UNIX)
    set_target_properties(sedov_taylor_2d_gdisph_plugin PROPERTIES
        SUFFIX ".so"
    )
endif()

message(STATUS "Building sedov_taylor_2d_gdisph plugin for DIM=${BUILD_DIM}")
