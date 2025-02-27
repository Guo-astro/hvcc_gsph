cmake_minimum_required(VERSION 3.23)
project(gsphcode CXX)
set(CMAKE_OSX_ARCHITECTURES "arm64")

# If you use OpenMP
find_package(OpenMP REQUIRED)

# If your code uses property_tree, format, etc. and needs linking:
# (If you only need header-only parts of Boost, you can leave out COMPONENTS or reduce it.)
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
    # If you actually need a specific Boost library:
    Boost::filesystem       # e.g. for <boost/filesystem.hpp> usage
    Boost::iostreams        # e.g. if needed by property_tree JSON
    # Or if everything you need is purely header-only, you might only do:
    # Boost::boost
)

# Add subdirectories for source
add_subdirectory(include)
add_subdirectory(src)
