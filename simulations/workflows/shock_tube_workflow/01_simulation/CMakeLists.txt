cmake_minimum_required(VERSION 3.23)
project(shock_tube_plugin)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Paths
get_filename_component(SPH_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../../.." ABSOLUTE)
set(SPH_BUILD_DIR "${SPH_ROOT}/build")
set(SPH_INCLUDE_DIR "${SPH_ROOT}/include")

include_directories(${SPH_INCLUDE_DIR})
add_compile_definitions(DIM=1)

# Main plugin from src/
add_library(shock_tube_plugin SHARED src/plugin.cpp)
target_link_libraries(shock_tube_plugin PRIVATE ${SPH_BUILD_DIR}/libsph_lib.a)

set_target_properties(shock_tube_plugin PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/build"
)
