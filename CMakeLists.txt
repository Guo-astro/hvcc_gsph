cmake_minimum_required(VERSION 3.23)
project(gsphcode CXX)
set(CMAKE_OSX_ARCHITECTURES "arm64")


# If you use OpenMP
find_package(OpenMP REQUIRED)

# Find Boost libraries (filesystem and iostreams in this case)
find_package(Boost REQUIRED COMPONENTS filesystem iostreams)

add_executable(sph)  # Our main executable
target_compile_features(sph PUBLIC cxx_std_14)

target_compile_options(sph
    PUBLIC
    -Wall
    -Wno-sign-compare
    -Wno-maybe-uninitialized
    -funroll-loops
    -ffast-math
)

# Where to look for headers
target_include_directories(sph PUBLIC include)

# Link libraries
target_link_libraries(sph
    PUBLIC
    OpenMP::OpenMP_CXX
    Boost::filesystem
    Boost::iostreams
)

# Add subdirectories for source
add_subdirectory(include)
add_subdirectory(src)
