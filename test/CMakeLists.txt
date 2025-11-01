# Google Test setup for SPH project

# Fetch Google Test
include(FetchContent)
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG        v1.14.0
)

# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# Enable testing
enable_testing()

# Include directories for tests
include_directories(${CMAKE_SOURCE_DIR}/include)

# Add test subdirectories
add_subdirectory(unit_tests)

# Legacy kernel test (keep for now)
add_subdirectory(kernel_test)
