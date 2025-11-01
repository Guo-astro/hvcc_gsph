/**
 * @file test_particle.cpp
 * @brief Unit tests for SPH particle structure
 */

#include <gtest/gtest.h>
#include "core/particle.hpp"
#include "utilities/defines.hpp"

namespace sph {
namespace test {

/**
 * @brief Test fixture for SPH particle tests
 */
class ParticleTest : public ::testing::Test {
protected:
    SPHParticle particle;
    
    void SetUp() override {
        // Initialize with default values
        particle.pos = vec_t(0.0, 0.0, 0.0);
        particle.vel = vec_t(0.0, 0.0, 0.0);
        particle.mass = 1.0;
        particle.dens = 1.0;
        particle.pres = 1.0;
        particle.ene = 1.0;
        particle.sml = 1.0;
        particle.id = 0;
    }
};

/**
 * @brief Test particle initialization
 */
TEST_F(ParticleTest, Initialization) {
    EXPECT_EQ(particle.mass, 1.0);
    EXPECT_EQ(particle.dens, 1.0);
    EXPECT_EQ(particle.pres, 1.0);
    EXPECT_EQ(particle.ene, 1.0);
    EXPECT_EQ(particle.sml, 1.0);
}

/**
 * @brief Test particle position and velocity
 */
TEST_F(ParticleTest, PositionVelocity) {
    particle.pos[0] = 1.0;
    particle.pos[1] = 2.0;
    particle.pos[2] = 3.0;
    
    EXPECT_EQ(particle.pos[0], 1.0);
    EXPECT_EQ(particle.pos[1], 2.0);
    EXPECT_EQ(particle.pos[2], 3.0);
    
    particle.vel[0] = 0.5;
    EXPECT_EQ(particle.vel[0], 0.5);
}

/**
 * @brief Test particle thermodynamic properties
 */
TEST_F(ParticleTest, ThermodynamicProperties) {
    const real gamma = 1.4;
    particle.dens = 2.0;
    particle.pres = 1.5;
    
    // Calculate energy from pressure and density
    particle.ene = particle.pres / ((gamma - 1.0) * particle.dens);
    
    // Verify ideal gas EOS
    real expected_ene = 1.5 / (0.4 * 2.0);
    EXPECT_NEAR(particle.ene, expected_ene, 1e-6);
}

/**
 * @brief Test particle shock detection flags
 */
TEST_F(ParticleTest, ShockDetection) {
    particle.shockMode = 0;
    particle.oldShockMode = 0;
    particle.shockSensor = 0.0;
    
    EXPECT_EQ(particle.shockMode, 0);
    EXPECT_EQ(particle.oldShockMode, 0);
    EXPECT_EQ(particle.shockSensor, 0.0);
    
    // Simulate shock detection
    particle.shockSensor = 1.5;
    particle.shockMode = 1;
    
    EXPECT_EQ(particle.shockMode, 1);
    EXPECT_GT(particle.shockSensor, 1.0);
}

/**
 * @brief Test particle flags
 */
TEST_F(ParticleTest, ParticleFlags) {
    EXPECT_FALSE(particle.is_wall);
    EXPECT_FALSE(particle.is_point_mass);
    
    particle.is_wall = true;
    EXPECT_TRUE(particle.is_wall);
    
    particle.is_point_mass = true;
    EXPECT_TRUE(particle.is_point_mass);
}

/**
 * @brief Test particle energy floor flag
 */
TEST_F(ParticleTest, EnergyFloor) {
    particle.ene_floored = 0;
    EXPECT_EQ(particle.ene_floored, 0);
    
    particle.ene_floored = 1;
    EXPECT_EQ(particle.ene_floored, 1);
}

} // namespace test
} // namespace sph
