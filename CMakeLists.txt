cmake_minimum_required(VERSION 3.23)
project(gsphcode CXX)

# ============================================================================
# Build Dimension Configuration
# ============================================================================
# Use set() with CACHE instead of option() to allow integer values
set(BUILD_DIM "2" CACHE STRING "Spatial dimension (1, 2, or 3)")

# Validate dimension
if(NOT BUILD_DIM MATCHES "^[123]$")
    message(FATAL_ERROR "BUILD_DIM must be 1, 2, or 3. Got: ${BUILD_DIM}")
endif()

message(STATUS "========================================")
message(STATUS "Building for ${BUILD_DIM}D simulations")
message(STATUS "========================================")

# Define DIM at compile time
add_compile_definitions(DIM=${BUILD_DIM})

# ============================================================================
# Compiler Settings
# ============================================================================
# Set architecture for macOS arm64 and enforce C++14
set(CMAKE_OSX_ARCHITECTURES "arm64")
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(OpenMP REQUIRED)
find_package(Boost REQUIRED COMPONENTS filesystem iostreams)
find_package(OpenSSL REQUIRED)

# Create a static library target for our project sources.
# Using STATIC instead of SHARED because DIM is a compile-time constant
add_library(sph_lib STATIC ${SOURCE_FILES} test/kernel_test/kernel_test.cpp)
target_include_directories(sph_lib PUBLIC ${CMAKE_SOURCE_DIR}/include ${Boost_INCLUDE_DIRS})
target_compile_options(sph_lib PUBLIC
    -Wall
    -Wno-sign-compare
    -Wno-maybe-uninitialized
    -funroll-loops
    -ffast-math
)
target_link_libraries(sph_lib PUBLIC OpenMP::OpenMP_CXX Boost::filesystem Boost::iostreams OpenSSL::Crypto)

# Add subdirectories (each will add its sources to sph_lib)
add_subdirectory(include)
add_subdirectory(src)
add_subdirectory(test)

# Build the main executable from src/core/main.cpp and link to sph_lib.
# Create dimension-specific executable: sph1d, sph2d, or sph3d
add_executable(sph${BUILD_DIM}d src/core/main.cpp)

# Force-link the entire static library to ensure global constructors run
# This is necessary for the sample registry to be populated
if(APPLE)
    target_link_libraries(sph${BUILD_DIM}d PRIVATE -Wl,-force_load,$<TARGET_FILE:sph_lib>)
    # Still need to link sph_lib to get include directories and other dependencies
    target_link_libraries(sph${BUILD_DIM}d PRIVATE sph_lib)
else()
    target_link_libraries(sph${BUILD_DIM}d PRIVATE -Wl,--whole-archive sph_lib -Wl,--no-whole-archive)
endif()

set_target_properties(sph${BUILD_DIM}d PROPERTIES MACOSX_RPATH YES)

add_custom_target(run_kernel_test
    COMMAND ./sph${BUILD_DIM}d kernel_test
    DEPENDS sph${BUILD_DIM}d
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)
