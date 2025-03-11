cmake_minimum_required(VERSION 3.23)
project(gsphcode CXX)

# Set architecture for macOS arm64 and enforce C++14
set(CMAKE_OSX_ARCHITECTURES "arm64")
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(OpenMP REQUIRED)
find_package(Boost REQUIRED COMPONENTS filesystem iostreams)

# Create a shared library target for our project sources.
add_library(sph_lib SHARED ${SOURCE_FILES} test/kernel_test/kernel_test.cpp)
target_include_directories(sph_lib PUBLIC ${CMAKE_SOURCE_DIR}/include ${Boost_INCLUDE_DIRS})
target_compile_options(sph_lib PUBLIC
    -Wall
    -Wno-sign-compare
    -Wno-maybe-uninitialized
    -funroll-loops
    -ffast-math
)
target_link_libraries(sph_lib PUBLIC OpenMP::OpenMP_CXX Boost::filesystem Boost::iostreams)

# Add subdirectories (each will add its sources to sph_lib)
add_subdirectory(include)
add_subdirectory(src)
add_subdirectory(test)

# Build the main executable from src/main.cpp and link to sph_lib.
add_executable(sph src/main.cpp)
target_link_libraries(sph PRIVATE sph_lib)
set_target_properties(sph PROPERTIES MACOSX_RPATH YES)

add_custom_target(run_kernel_test
    COMMAND ./sph kernel_test
    DEPENDS sph
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)
