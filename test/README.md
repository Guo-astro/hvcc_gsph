# SPH Unit Tests

This directory contains unit tests for the SPH simulation code using Google Test framework.

## Test Structure

```
test/
├── CMakeLists.txt           # Main test configuration
├── unit_tests/              # Google Test unit tests
│   ├── CMakeLists.txt
│   ├── test_kernel.cpp      # Kernel function tests
│   ├── test_particle.cpp    # Particle structure tests
│   ├── test_simulation.cpp  # Simulation class tests
│   └── test_vector_math.cpp # Vector math operations tests
└── kernel_test/             # Legacy kernel test (will be migrated)
    ├── CMakeLists.txt
    └── kernel_test.cpp
```

## Building Tests

### Build all tests:
```bash
cd build
cmake ..
make sph_unit_tests
```

### Run all tests:
```bash
# Using CTest
cd build
ctest --output-on-failure

# Or run directly
./sph_unit_tests
```

### Run specific test suite:
```bash
./sph_unit_tests --gtest_filter=CubicSplineTest.*
./sph_unit_tests --gtest_filter=ParticleTest.*
```

### Run with verbose output:
```bash
./sph_unit_tests --gtest_verbose
```

## Test Coverage

### Current Tests:

#### Kernel Tests (`test_kernel.cpp`)
- ✅ Cubic spline kernel value beyond support radius
- ✅ Kernel symmetry
- ✅ Kernel normalization
- ✅ Kernel derivative accuracy (compared to numerical differentiation)
- ✅ H-derivative accuracy
- ✅ Wendland C4 kernel tests
- ✅ Kernel smoothness at boundaries

#### Particle Tests (`test_particle.cpp`)
- ✅ Particle initialization
- ✅ Position and velocity handling
- ✅ Thermodynamic properties (density, pressure, energy)
- ✅ Shock detection flags
- ✅ Particle flags (wall, point mass)
- ✅ Energy floor tracking

#### Simulation Tests (`test_simulation.cpp`)
- ✅ Simulation initialization
- ✅ Particle addition and access
- ✅ Time parameters
- ✅ CFL parameters
- ✅ Artificial viscosity parameters
- ✅ Physics parameters

#### Vector Math Tests (`test_vector_math.cpp`)
- ✅ Vector initialization
- ✅ Vector addition/subtraction
- ✅ Scalar multiplication/division
- ✅ Dot product
- ✅ Vector magnitude and normalization
- ✅ Cross product
- ✅ Distance calculations

## Adding New Tests

### 1. Create a new test file:
```cpp
#include <gtest/gtest.h>
#include "your_header.hpp"

namespace sph {
namespace test {

class YourTestFixture : public ::testing::Test {
protected:
    void SetUp() override {
        // Setup code
    }
};

TEST_F(YourTestFixture, TestName) {
    // Test code
    EXPECT_EQ(actual, expected);
}

} // namespace test
} // namespace sph
```

### 2. Add to CMakeLists.txt:
```cmake
add_executable(sph_unit_tests
    # ... existing files ...
    test_your_feature.cpp
)
```

### 3. Build and run:
```bash
cd build
make sph_unit_tests
./sph_unit_tests
```

## Google Test Assertions

### Common Assertions:
- `EXPECT_EQ(a, b)` - Expect equality
- `EXPECT_NE(a, b)` - Expect inequality
- `EXPECT_LT/LE/GT/GE(a, b)` - Comparison
- `EXPECT_NEAR(a, b, tol)` - Floating point comparison with tolerance
- `EXPECT_TRUE/FALSE(cond)` - Boolean checks
- `ASSERT_*` - Same as EXPECT but fails immediately

### Example Usage:
```cpp
EXPECT_EQ(particle.mass, 1.0);
EXPECT_NEAR(kernel.w(r, h), expected, 1e-6);
EXPECT_TRUE(param->av.use_balsara_switch);
```

## Continuous Integration

Tests should be run automatically on every commit:
```bash
# In CI pipeline
cd build
cmake ..
make sph_unit_tests
ctest --output-on-failure --verbose
```

## Future Test Additions

### Priority Tests to Add:
- [ ] Algorithm tests (GSPH, DISPH, GDISPH, SSPH)
- [ ] Tree structure tests (BH tree, neighbor search)
- [ ] Module tests (gravity, timestep, fluid force)
- [ ] Integration tests (shock tube, Sedov-Taylor)
- [ ] Conservation property tests (energy, momentum, mass)
- [ ] Boundary condition tests
- [ ] I/O tests (reading/writing snapshots)
- [ ] Configuration parser tests
- [ ] Sample registry tests

### Test Best Practices:
1. Each test should be independent
2. Use descriptive test names
3. Test both normal and edge cases
4. Keep tests fast (< 1 second each)
5. Use fixtures to reduce code duplication
6. Test one concept per test case
7. Add comments for complex test logic

## Debugging Failed Tests

### Run specific failing test:
```bash
./sph_unit_tests --gtest_filter=TestSuite.FailingTest
```

### Run with debugging:
```bash
gdb ./sph_unit_tests
(gdb) run --gtest_filter=TestSuite.FailingTest
```

### Get detailed output:
```bash
./sph_unit_tests --gtest_verbose --gtest_print_time=1
```

## References

- [Google Test Documentation](https://google.github.io/googletest/)
- [Google Test Primer](https://google.github.io/googletest/primer.html)
- [Google Test Advanced Guide](https://google.github.io/googletest/advanced.html)
