/**
 * @file test_vector_math.cpp
 * @brief Unit tests for vector math operations
 */

#include <gtest/gtest.h>
#include "utilities/vector_type.hpp"
#include "utilities/defines.hpp"
#include <cmath>

namespace sph {
namespace test {

/**
 * @brief Test fixture for vector math tests
 */
class VectorMathTest : public ::testing::Test {
protected:
    static constexpr real tolerance = 1e-10;
};

/**
 * @brief Test vector initialization
 */
TEST_F(VectorMathTest, VectorInitialization) {
#if DIM == 1
    vec_t v1(1.0);
    EXPECT_EQ(v1[0], 1.0);
#elif DIM == 2
    vec_t v1(1.0, 2.0);
    EXPECT_EQ(v1[0], 1.0);
    EXPECT_EQ(v1[1], 2.0);
#elif DIM == 3
    vec_t v1(1.0, 2.0, 3.0);
    EXPECT_EQ(v1[0], 1.0);
    EXPECT_EQ(v1[1], 2.0);
    EXPECT_EQ(v1[2], 3.0);
#endif
}

/**
 * @brief Test vector addition
 */
TEST_F(VectorMathTest, VectorAddition) {
#if DIM == 1
    vec_t v1(1.0);
    vec_t v2(4.0);
    vec_t result = v1 + v2;
    EXPECT_EQ(result[0], 5.0);
#elif DIM == 2
    vec_t v1(1.0, 2.0);
    vec_t v2(4.0, 5.0);
    vec_t result = v1 + v2;
    EXPECT_EQ(result[0], 5.0);
    EXPECT_EQ(result[1], 7.0);
#elif DIM == 3
    vec_t v1(1.0, 2.0, 3.0);
    vec_t v2(4.0, 5.0, 6.0);
    vec_t result = v1 + v2;
    EXPECT_EQ(result[0], 5.0);
    EXPECT_EQ(result[1], 7.0);
    EXPECT_EQ(result[2], 9.0);
#endif
}

/**
 * @brief Test vector subtraction
 */
TEST_F(VectorMathTest, VectorSubtraction) {
#if DIM == 1
    vec_t v1(5.0);
    vec_t v2(1.0);
    vec_t result = v1 - v2;
    EXPECT_EQ(result[0], 4.0);
#elif DIM == 2
    vec_t v1(5.0, 7.0);
    vec_t v2(1.0, 2.0);
    vec_t result = v1 - v2;
    EXPECT_EQ(result[0], 4.0);
    EXPECT_EQ(result[1], 5.0);
#elif DIM == 3
    vec_t v1(5.0, 7.0, 9.0);
    vec_t v2(1.0, 2.0, 3.0);
    vec_t result = v1 - v2;
    EXPECT_EQ(result[0], 4.0);
    EXPECT_EQ(result[1], 5.0);
    EXPECT_EQ(result[2], 6.0);
#endif
}

/**
 * @brief Test scalar multiplication
 */
TEST_F(VectorMathTest, ScalarMultiplication) {
#if DIM == 1
    vec_t v(1.0);
    real scalar = 2.5;
    vec_t result = v * scalar;
    EXPECT_EQ(result[0], 2.5);
#elif DIM == 2
    vec_t v(1.0, 2.0);
    real scalar = 2.5;
    vec_t result = v * scalar;
    EXPECT_EQ(result[0], 2.5);
    EXPECT_EQ(result[1], 5.0);
#elif DIM == 3
    vec_t v(1.0, 2.0, 3.0);
    real scalar = 2.5;
    vec_t result = v * scalar;
    EXPECT_EQ(result[0], 2.5);
    EXPECT_EQ(result[1], 5.0);
    EXPECT_EQ(result[2], 7.5);
#endif
}

/**
 * @brief Test scalar division
 */
TEST_F(VectorMathTest, ScalarDivision) {
#if DIM == 1
    vec_t v(10.0);
    real scalar = 2.0;
    vec_t result = v / scalar;
    EXPECT_EQ(result[0], 5.0);
#elif DIM == 2
    vec_t v(10.0, 20.0);
    real scalar = 2.0;
    vec_t result = v / scalar;
    EXPECT_EQ(result[0], 5.0);
    EXPECT_EQ(result[1], 10.0);
#elif DIM == 3
    vec_t v(10.0, 20.0, 30.0);
    real scalar = 2.0;
    vec_t result = v / scalar;
    EXPECT_EQ(result[0], 5.0);
    EXPECT_EQ(result[1], 10.0);
    EXPECT_EQ(result[2], 15.0);
#endif
}

/**
 * @brief Test dot product
 */
TEST_F(VectorMathTest, DotProduct) {
#if DIM == 1
    vec_t v1(2.0);
    vec_t v2(3.0);
    real dot = v1.dot(v2);
    real expected = 6.0;
    EXPECT_NEAR(dot, expected, tolerance);
#elif DIM == 2
    vec_t v1(1.0, 2.0);
    vec_t v2(3.0, 4.0);
    real dot = v1.dot(v2);
    real expected = 1.0*3.0 + 2.0*4.0;  // = 11.0
    EXPECT_NEAR(dot, expected, tolerance);
#elif DIM == 3
    vec_t v1(1.0, 2.0, 3.0);
    vec_t v2(4.0, 5.0, 6.0);
    real dot = v1.dot(v2);
    real expected = 1.0*4.0 + 2.0*5.0 + 3.0*6.0;  // = 32.0
    EXPECT_NEAR(dot, expected, tolerance);
#endif
}

/**
 * @brief Test vector magnitude
 */
TEST_F(VectorMathTest, VectorMagnitude) {
#if DIM == 1
    vec_t v(3.0);
    real mag = v.norm();
    EXPECT_NEAR(mag, 3.0, tolerance);
#elif DIM == 2
    vec_t v(3.0, 4.0);
    real mag = v.norm();
    real expected = std::sqrt(3.0*3.0 + 4.0*4.0);  // = 5.0
    EXPECT_NEAR(mag, expected, tolerance);
#elif DIM == 3
    vec_t v(3.0, 4.0, 0.0);
    real mag = v.norm();
    real expected = std::sqrt(3.0*3.0 + 4.0*4.0);  // = 5.0
    EXPECT_NEAR(mag, expected, tolerance);
#endif
}

/**
 * @brief Test vector normalization
 */
TEST_F(VectorMathTest, VectorNormalization) {
#if DIM == 1
    vec_t v(5.0);
    vec_t normalized = v.normalized();
    EXPECT_NEAR(normalized.norm(), 1.0, tolerance);
    EXPECT_NEAR(normalized[0], 1.0, tolerance);
#elif DIM == 2
    vec_t v(3.0, 4.0);
    vec_t normalized = v.normalized();
    EXPECT_NEAR(normalized.norm(), 1.0, tolerance);
    EXPECT_NEAR(normalized[0], 0.6, tolerance);  // 3/5
    EXPECT_NEAR(normalized[1], 0.8, tolerance);  // 4/5
#elif DIM == 3
    vec_t v(3.0, 4.0, 0.0);
    vec_t normalized = v.normalized();
    EXPECT_NEAR(normalized.norm(), 1.0, tolerance);
    EXPECT_NEAR(normalized[0], 0.6, tolerance);  // 3/5
    EXPECT_NEAR(normalized[1], 0.8, tolerance);  // 4/5
#endif
}

/**
 * @brief Test zero vector
 */
TEST_F(VectorMathTest, ZeroVector) {
#if DIM == 1
    vec_t v(0.0);
    EXPECT_EQ(v[0], 0.0);
    EXPECT_EQ(v.norm(), 0.0);
#elif DIM == 2
    vec_t v(0.0, 0.0);
    EXPECT_EQ(v[0], 0.0);
    EXPECT_EQ(v[1], 0.0);
    EXPECT_EQ(v.norm(), 0.0);
#elif DIM == 3
    vec_t v(0.0, 0.0, 0.0);
    EXPECT_EQ(v[0], 0.0);
    EXPECT_EQ(v[1], 0.0);
    EXPECT_EQ(v[2], 0.0);
    EXPECT_EQ(v.norm(), 0.0);
#endif
}

/**
 * @brief Test vector cross product (3D only)
 */
TEST_F(VectorMathTest, CrossProduct) {
#if DIM == 3
    vec_t v1(1.0, 0.0, 0.0);
    vec_t v2(0.0, 1.0, 0.0);
    
    // Cross product of unit x and unit y should be unit z
    vec_t result = v1.cross(v2);
    
    EXPECT_NEAR(result[0], 0.0, tolerance);
    EXPECT_NEAR(result[1], 0.0, tolerance);
    EXPECT_NEAR(result[2], 1.0, tolerance);
#else
    // Cross product only defined in 3D - skip test
    GTEST_SKIP() << "Cross product only defined for 3D vectors";
#endif
}

/**
 * @brief Test distance calculation
 */
TEST_F(VectorMathTest, Distance) {
#if DIM == 1
    vec_t p1(0.0);
    vec_t p2(5.0);
    vec_t diff = p2 - p1;
    real distance = diff.norm();
    EXPECT_NEAR(distance, 5.0, tolerance);
#elif DIM == 2
    vec_t p1(0.0, 0.0);
    vec_t p2(3.0, 4.0);
    vec_t diff = p2 - p1;
    real distance = diff.norm();
    EXPECT_NEAR(distance, 5.0, tolerance);
#elif DIM == 3
    vec_t p1(0.0, 0.0, 0.0);
    vec_t p2(3.0, 4.0, 0.0);
    vec_t diff = p2 - p1;
    real distance = diff.norm();
    EXPECT_NEAR(distance, 5.0, tolerance);
#endif
}

} // namespace test
} // namespace sph
