/**
 * @file test_basic.cpp
 * @brief Basic tests for SPH core structures
 */

#include <gtest/gtest.h>
#include "utilities/vector_type.hpp"
#include "utilities/defines.hpp"
#include "core/particle.hpp"

namespace sph {
namespace test {

/**
 * @brief Test fixture for basic SPH operations
 */
class BasicSPHTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Setup code
    }

    void TearDown() override {
        // Cleanup code
    }
};

/**
 * @brief Test vector type initialization
 */
TEST_F(BasicSPHTest, VectorInitialization) {
#if DIM == 1
    vec_t v(1.0);
    EXPECT_EQ(v[0], 1.0);
#elif DIM == 2
    vec_t v(1.0, 2.0);
    EXPECT_EQ(v[0], 1.0);
    EXPECT_EQ(v[1], 2.0);
#elif DIM == 3
    vec_t v(1.0, 2.0, 3.0);
    EXPECT_EQ(v[0], 1.0);
    EXPECT_EQ(v[1], 2.0);
    EXPECT_EQ(v[2], 3.0);
#endif
}

/**
 * @brief Test vector operations
 */
TEST_F(BasicSPHTest, VectorOperations) {
#if DIM == 1
    vec_t v1(2.0);
    vec_t v2(3.0);
    
    vec_t sum = v1 + v2;
    EXPECT_EQ(sum[0], 5.0);
    
    vec_t diff = v2 - v1;
    EXPECT_EQ(diff[0], 1.0);
    
    vec_t scaled = v1 * 2.5;
    EXPECT_EQ(scaled[0], 5.0);
#elif DIM == 2
    vec_t v1(1.0, 2.0);
    vec_t v2(3.0, 4.0);
    
    vec_t sum = v1 + v2;
    EXPECT_EQ(sum[0], 4.0);
    EXPECT_EQ(sum[1], 6.0);
    
    vec_t diff = v2 - v1;
    EXPECT_EQ(diff[0], 2.0);
    EXPECT_EQ(diff[1], 2.0);
    
    vec_t scaled = v1 * 2.0;
    EXPECT_EQ(scaled[0], 2.0);
    EXPECT_EQ(scaled[1], 4.0);
#elif DIM == 3
    vec_t v1(1.0, 2.0, 3.0);
    vec_t v2(4.0, 5.0, 6.0);
    
    vec_t sum = v1 + v2;
    EXPECT_EQ(sum[0], 5.0);
    EXPECT_EQ(sum[1], 7.0);
    EXPECT_EQ(sum[2], 9.0);
    
    vec_t diff = v2 - v1;
    EXPECT_EQ(diff[0], 3.0);
    EXPECT_EQ(diff[1], 3.0);
    EXPECT_EQ(diff[2], 3.0);
    
    vec_t scaled = v1 * 2.0;
    EXPECT_EQ(scaled[0], 2.0);
    EXPECT_EQ(scaled[1], 4.0);
    EXPECT_EQ(scaled[2], 6.0);
#endif
}

/**
 * @brief Test inner product function
 */
TEST_F(BasicSPHTest, InnerProduct) {
#if DIM == 1
    vec_t v1(2.0);
    vec_t v2(3.0);
    real dot = inner_product(v1, v2);
    EXPECT_EQ(dot, 6.0);
#elif DIM == 2
    vec_t v1(1.0, 2.0);
    vec_t v2(3.0, 4.0);
    real dot = inner_product(v1, v2);
    EXPECT_EQ(dot, 11.0);  // 1*3 + 2*4
#elif DIM == 3
    vec_t v1(1.0, 2.0, 3.0);
    vec_t v2(4.0, 5.0, 6.0);
    real dot = inner_product(v1, v2);
    EXPECT_EQ(dot, 32.0);  // 1*4 + 2*5 + 3*6
#endif
}

/**
 * @brief Test vector magnitude
 */
TEST_F(BasicSPHTest, VectorMagnitude) {
    const real tolerance = 1e-6;
    
#if DIM == 1
    vec_t v(3.0);
    real mag = std::abs(v);
    EXPECT_NEAR(mag, 3.0, tolerance);
#elif DIM == 2
    vec_t v(3.0, 4.0);
    real mag = std::abs(v);
    EXPECT_NEAR(mag, 5.0, tolerance);  // 3-4-5 triangle
#elif DIM == 3
    vec_t v(1.0, 2.0, 2.0);
    real mag = std::abs(v);
    EXPECT_NEAR(mag, 3.0, tolerance);  // sqrt(1 + 4 + 4)
#endif
}

/**
 * @brief Test SPH particle initialization
 */
TEST_F(BasicSPHTest, ParticleInitialization) {
    SPHParticle p;
    
    // Test that we can set properties
#if DIM == 1
    p.pos = vec_t(1.0);
    p.vel = vec_t(0.5);
    EXPECT_EQ(p.pos[0], 1.0);
    EXPECT_EQ(p.vel[0], 0.5);
#elif DIM == 2
    p.pos = vec_t(1.0, 2.0);
    p.vel = vec_t(0.5, -0.5);
    EXPECT_EQ(p.pos[0], 1.0);
    EXPECT_EQ(p.pos[1], 2.0);
    EXPECT_EQ(p.vel[0], 0.5);
    EXPECT_EQ(p.vel[1], -0.5);
#elif DIM == 3
    p.pos = vec_t(1.0, 2.0, 3.0);
    p.vel = vec_t(0.5, -0.5, 1.0);
    EXPECT_EQ(p.pos[0], 1.0);
    EXPECT_EQ(p.pos[1], 2.0);
    EXPECT_EQ(p.pos[2], 3.0);
    EXPECT_EQ(p.vel[0], 0.5);
    EXPECT_EQ(p.vel[1], -0.5);
    EXPECT_EQ(p.vel[2], 1.0);
#endif
    
    // Test thermodynamic properties (use actual field names)
    p.dens = 1.0;
    p.pres = 0.5;
    p.ene = 1.5;
    p.mass = 2.0;
    
    EXPECT_EQ(p.dens, 1.0);
    EXPECT_EQ(p.pres, 0.5);
    EXPECT_EQ(p.ene, 1.5);
    EXPECT_EQ(p.mass, 2.0);
}

} // namespace test
} // namespace sph
