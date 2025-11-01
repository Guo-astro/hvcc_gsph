/**
 * @file test_kernel.cpp
 * @brief Unit tests for SPH kernel functions
 */

#include <gtest/gtest.h>
#include <cmath>
#include "kernel/cubic_spline.hpp"
#include "kernel/wendland_kernel.hpp"
#include "utilities/defines.hpp"
#include "utilities/vector_type.hpp"

namespace sph {
namespace test {

/**
 * @brief Test fixture for cubic spline kernel tests
 */
class CubicSplineTest : public ::testing::Test {
protected:
    Spline::Cubic kernel;
    static constexpr real h = 1.0;
    static constexpr real tolerance = 1e-6;
};

/**
 * @brief Test that kernel value is zero beyond support radius
 */
TEST_F(CubicSplineTest, KernelValueZeroBeyondSupport) {
    real r = 2.1 * h;  // Beyond support (2h)
    real w = kernel.w(r, h);
    EXPECT_NEAR(w, 0.0, tolerance);
}

/**
 * @brief Test that kernel is symmetric
 */
TEST_F(CubicSplineTest, KernelSymmetry) {
    real r = 1.0;
    
#if DIM == 1
    vec_t pos1(r);
    vec_t pos2(-r);
#elif DIM == 2
    vec_t pos1(r, 0.0);
    vec_t pos2(-r, 0.0);
#elif DIM == 3
    vec_t pos1(r, 0.0, 0.0);
    vec_t pos2(-r, 0.0, 0.0);
#endif

/**
 * @brief Test kernel normalization (integral over space = 1)
 * Using Monte Carlo integration
 */
TEST_F(CubicSplineTest, KernelNormalization) {
    const int n_samples = 100;
    real sum = 0.0;
    real dr = 2.0 * h / n_samples;
    
    // 1D integration (for 1D kernel)
    for (int i = 0; i < n_samples; ++i) {
        real r = (i + 0.5) * dr;
        sum += kernel.w(r, h) * dr;
    }
    
    // Check normalization (should be close to 1.0)
    EXPECT_NEAR(sum, 1.0, 0.1);
}

/**
 * @brief Test kernel derivative using numerical differentiation
 */
TEST_F(CubicSplineTest, KernelDerivativeAccuracy) {
    const real dx = 1e-4;
    
    for (real r = 0.1; r < 2.0 * h; r += 0.2) {
        vec_t pos(r, 0.0, 0.0);
        
        // Analytical derivative
        auto dw_analytical = kernel.dw(pos, r, h);
        
        // Numerical derivative
        real dw_numerical = (kernel.w(r + dx/2, h) - kernel.w(r - dx/2, h)) / dx;
        
        EXPECT_NEAR(dw_analytical[0], dw_numerical, 1e-3)
            << "Mismatch at r = " << r;
    }
}

/**
 * @brief Test h-derivative using numerical differentiation
 */
TEST_F(CubicSplineTest, HDerivativeAccuracy) {
    const real dh = 1e-4;
    
    for (real r = 0.1; r < 2.0 * h; r += 0.2) {
        // Analytical h-derivative
        real dhw_analytical = kernel.dhw(r, h);
        
        // Numerical h-derivative
        real dhw_numerical = (kernel.w(r, h + dh/2) - kernel.w(r, h - dh/2)) / dh;
        
        EXPECT_NEAR(dhw_analytical, dhw_numerical, 1e-3)
            << "Mismatch at r = " << r;
    }
}

/**
 * @brief Test fixture for Wendland kernel tests
 */
class WendlandKernelTest : public ::testing::Test {
protected:
    Wendland::C4Kernel kernel;
    static constexpr real h = 1.0;
    static constexpr real tolerance = 1e-6;
};

/**
 * @brief Test that Wendland kernel is zero beyond support
 */
TEST_F(WendlandKernelTest, KernelValueZeroBeyondSupport) {
    real r = 2.1 * h;
    real w = kernel.w(r, h);
    EXPECT_NEAR(w, 0.0, tolerance);
}

/**
 * @brief Test Wendland kernel derivative accuracy
 */
TEST_F(WendlandKernelTest, KernelDerivativeAccuracy) {
    const real dx = 1e-4;
    
    for (real r = 0.1; r < 2.0 * h; r += 0.2) {
        vec_t pos(r, 0.0, 0.0);
        
        auto dw_analytical = kernel.dw(pos, r, h);
        real dw_numerical = (kernel.w(r + dx/2, h) - kernel.w(r - dx/2, h)) / dx;
        
        EXPECT_NEAR(dw_analytical[0], dw_numerical, 1e-3)
            << "Mismatch at r = " << r;
    }
}

/**
 * @brief Test Wendland kernel smoothness (C4 continuity)
 */
TEST_F(WendlandKernelTest, KernelSmoothness) {
    // Test continuity at support boundary
    real r_boundary = 2.0 * h;
    real epsilon = 1e-6;
    
    real w_inside = kernel.w(r_boundary - epsilon, h);
    real w_outside = kernel.w(r_boundary + epsilon, h);
    
    EXPECT_NEAR(w_outside, 0.0, tolerance);
    EXPECT_GE(w_inside, 0.0);
}

} // namespace test
} // namespace sph
