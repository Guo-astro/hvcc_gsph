/**
 * @file test_simulation.cpp
 * @brief Unit tests for Simulation class
 */

#include <gtest/gtest.h>
#include "core/simulation.hpp"
#include "core/parameters.hpp"
#include "core/particle.hpp"
#include <memory>
#include <vector>

namespace sph {
namespace test {

/**
 * @brief Test fixture for Simulation tests
 */
class SimulationTest : public ::testing::Test {
protected:
    std::shared_ptr<Simulation> sim;
    std::shared_ptr<SPHParameters> param;
    
    void SetUp() override {
        sim = std::make_shared<Simulation>();
        param = std::make_shared<SPHParameters>();
    }
};

/**
 * @brief Test simulation initialization
 */
TEST_F(SimulationTest, Initialization) {
    ASSERT_NE(sim, nullptr);
    ASSERT_NE(param, nullptr);
}

/**
 * @brief Test particle addition to simulation
 */
TEST_F(SimulationTest, AddParticles) {
    std::vector<SPHParticle> particles(10);
    
    // Initialize particles
    for (size_t i = 0; i < particles.size(); ++i) {
        particles[i].id = i;
        particles[i].pos[0] = static_cast<real>(i) * 0.1;
        particles[i].mass = 1.0;
        particles[i].dens = 1.0;
    }
    
    sim->set_particles(particles);
    sim->set_particle_num(particles.size());
    
    EXPECT_EQ(sim->get_particle_num(), 10);
}

/**
 * @brief Test particle properties access
 */
TEST_F(SimulationTest, ParticlePropertiesAccess) {
    std::vector<SPHParticle> particles(5);
    
    for (size_t i = 0; i < particles.size(); ++i) {
        particles[i].id = i;
        particles[i].mass = 2.0;
        particles[i].dens = 1.5;
        particles[i].pres = 1.0;
    }
    
    sim->set_particles(particles);
    sim->set_particle_num(particles.size());
    
    auto& sim_particles = sim->get_particles();
    EXPECT_EQ(sim_particles.size(), 5);
    EXPECT_EQ(sim_particles[0].mass, 2.0);
    EXPECT_EQ(sim_particles[0].dens, 1.5);
}

/**
 * @brief Test simulation time properties
 */
TEST_F(SimulationTest, TimeProperties) {
    param->time.start = 0.0;
    param->time.end = 1.0;
    param->time.output = 0.1;
    
    EXPECT_EQ(param->time.start, 0.0);
    EXPECT_EQ(param->time.end, 1.0);
    EXPECT_EQ(param->time.output, 0.1);
    EXPECT_LT(param->time.start, param->time.end);
}

/**
 * @brief Test CFL parameters
 */
TEST_F(SimulationTest, CFLParameters) {
    param->cfl.sound = 0.3;
    param->cfl.force = 0.125;
    param->cfl.ene = 0.3;
    
    EXPECT_GT(param->cfl.sound, 0.0);
    EXPECT_GT(param->cfl.force, 0.0);
    EXPECT_LE(param->cfl.sound, 1.0);
    EXPECT_LE(param->cfl.force, 1.0);
}

/**
 * @brief Test artificial viscosity parameters
 */
TEST_F(SimulationTest, ArtificialViscosity) {
    param->av.alpha = 1.0;
    param->av.use_balsara_switch = true;
    param->av.use_time_dependent_av = false;
    
    EXPECT_EQ(param->av.alpha, 1.0);
    EXPECT_TRUE(param->av.use_balsara_switch);
    EXPECT_FALSE(param->av.use_time_dependent_av);
}

/**
 * @brief Test physics parameters
 */
TEST_F(SimulationTest, PhysicsParameters) {
    param->physics.gamma = 1.4;
    param->physics.neighbor_number = 32;
    
    EXPECT_EQ(param->physics.gamma, 1.4);
    EXPECT_EQ(param->physics.neighbor_number, 32);
    EXPECT_GT(param->physics.gamma, 1.0);
}

} // namespace test
} // namespace sph
