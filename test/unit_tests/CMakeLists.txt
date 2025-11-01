# Unit tests with Google Test

# Create test executable
add_executable(sph_unit_tests
    test_basic.cpp
    test_disph_simple.cpp
    checkpoint_basic_test.cpp
    checkpoint_data_test.cpp
    checkpoint_load_test.cpp
    checkpoint_integration_test.cpp
    # test_disph.cpp  # TODO: Fix to match codebase API
    # test_disph_integration.cpp  # TODO: Fix to match codebase API
)

# Link against gtest, gtest_main, and the SPH library
target_link_libraries(sph_unit_tests
    PRIVATE
    gtest
    gtest_main
    sph_lib
)

# Discover tests for CTest
include(GoogleTest)
gtest_discover_tests(sph_unit_tests)
