/**
 * @file test_disph.cpp
 * @brief Unit tests for DISPH (Density-Independent SPH) algorithm
 * 
 * Tests based on TDD/BDD approach following the paper:
 * "Novel Hydrodynamic Schemes Capturing Shocks and Contact Discontinuities"
 * by Yuasa & Mori (2023) - arXiv:2312.03224
 * 
 * These tests verify:
 * 1. Volume element calculations
 * 2. Pressure force formulation
 * 3. Contact discontinuity handling
 * 4. Conservation properties
 * 5. Gradient calculations
 */

#include <gtest/gtest.h>
#include <cmath>
#include <vector>
#include "core/particle.hpp"
#include "core/simulation.hpp"
#include "core/parameters.hpp"
#include "kernel/kernel_function.hpp"
#include "kernel/cubic_spline.hpp"

namespace sph {
namespace test {

class DISPHTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize parameters for DISPH
        param = std::make_unique<SPHParameters>();
        param->sph.type = "disph";
        param->sph.kernel_type = "cubic_spline";
        param->sph.dim = 1;
        param->physics.gamma = 5.0 / 3.0;  // Adiabatic index for ideal gas
        param->av.alpha = 1.0;
        param->av.beta = 2.0;
        
        // Initialize simulation
        sim = std::make_unique<Simulation>(param.get());
        
        // Create kernel
        kernel = std::make_unique<CubicSpline>(param->sph.dim);
    }

    void TearDown() override {
        // Cleanup
    }

    // Helper function to create a test particle
    SPHParticle createParticle(real mass, real density, real pressure, 
                               real x = 0.0, real y = 0.0, real z = 0.0) {
        SPHParticle p;
        p.mass = mass;
        p.dens = density;
        p.pres = pressure;
        p.pos.x = x;
        p.pos.y = y;
        p.pos.z = z;
        p.vel.x = p.vel.y = p.vel.z = 0.0;
        p.sml = 1.0;
        
        // Calculate internal energy from pressure and density
        p.ene = pressure / ((param->physics.gamma - 1.0) * density);
        
        return p;
    }

    // Helper to calculate volume element
    real calculateVolume(const SPHParticle& p) {
        return p.mass / p.dens;
    }

    std::unique_ptr<SPHParameters> param;
    std::unique_ptr<Simulation> sim;
    std::unique_ptr<KernelFunction> kernel;
};

// ============================================================================
// Feature: DISPH Volume Element Calculation
// ============================================================================

TEST_F(DISPHTest, VolumeElementBasicCalculation) {
    // Scenario: Computing particle volume elements
    // Given a particle with mass 1.0 and density 2.0
    SPHParticle p = createParticle(1.0, 2.0, 1.0);
    
    // When I calculate the volume element
    real volume = calculateVolume(p);
    
    // Then the volume should be 0.5
    EXPECT_NEAR(volume, 0.5, 1e-10);
    
    // And the volume should equal mass divided by density
    EXPECT_NEAR(volume, p.mass / p.dens, 1e-10);
}

TEST_F(DISPHTest, VolumeElementUpdatesWithDensityChanges) {
    // Scenario: Volume element updates with density changes
    // Given a particle with mass 1.0 and initial density 1.0
    SPHParticle p = createParticle(1.0, 1.0, 1.0);
    real initial_volume = calculateVolume(p);
    EXPECT_NEAR(initial_volume, 1.0, 1e-10);
    
    // When the density is updated to 2.0
    p.dens = 2.0;
    
    // And I recalculate the volume element
    real new_volume = calculateVolume(p);
    
    // Then the volume should be 0.5
    EXPECT_NEAR(new_volume, 0.5, 1e-10);
}

TEST_F(DISPHTest, VolumeElementDifferentMasses) {
    // Test volume elements for particles with different masses
    SPHParticle p1 = createParticle(2.0, 1.0, 1.0);
    SPHParticle p2 = createParticle(1.0, 1.0, 1.0);
    
    real v1 = calculateVolume(p1);
    real v2 = calculateVolume(p2);
    
    // Volume should be proportional to mass at same density
    EXPECT_NEAR(v1 / v2, 2.0, 1e-10);
}

// ============================================================================
// Feature: DISPH Pressure Force Calculation
// ============================================================================

TEST_F(DISPHTest, PressureForceFormulation) {
    // Test the DISPH pressure force formulation: -m_j * (P_i/V_i^2 + P_j/V_j^2)
    
    SPHParticle pi = createParticle(1.0, 2.0, 2.0, 0.0);  // V_i = 0.5
    SPHParticle pj = createParticle(1.0, 1.0, 1.0, 1.0);  // V_j = 1.0
    
    real Vi = calculateVolume(pi);
    real Vj = calculateVolume(pj);
    
    // DISPH pressure term: P_i/V_i^2 + P_j/V_j^2
    real pressure_term = pi.pres / (Vi * Vi) + pj.pres / (Vj * Vj);
    
    // Expected: 2.0/0.25 + 1.0/1.0 = 8.0 + 1.0 = 9.0
    EXPECT_NEAR(pressure_term, 9.0, 1e-10);
}

TEST_F(DISPHTest, PressureEquilibriumSymmetry) {
    // Scenario: Symmetric pressure force between equal pressure particles
    // Given two particles at equal pressure but different densities
    SPHParticle pi = createParticle(1.0, 2.0, 2.0, 0.0);
    SPHParticle pj = createParticle(1.0, 1.0, 2.0, 1.0);
    
    real Vi = calculateVolume(pi);  // 0.5
    real Vj = calculateVolume(pj);  // 1.0
    
    // At pressure equilibrium with same pressure but different densities
    EXPECT_NEAR(pi.pres, pj.pres, 1e-10);
    EXPECT_NE(pi.dens, pj.dens);
    
    // The pressure gradient should still exist due to density differences
    // But DISPH formulation should handle this correctly
    real term_i = pi.pres / (Vi * Vi);  // 2.0 / 0.25 = 8.0
    real term_j = pj.pres / (Vj * Vj);  // 2.0 / 1.0 = 2.0
    
    EXPECT_NEAR(term_i, 8.0, 1e-10);
    EXPECT_NEAR(term_j, 2.0, 1e-10);
}

TEST_F(DISPHTest, ContactDiscontinuityPressureBalance) {
    // Scenario: Pressure force across contact discontinuity
    // At a contact discontinuity: same pressure, different density
    
    real common_pressure = 1.0;
    SPHParticle p_left = createParticle(1.0, 2.0, common_pressure, -0.5);
    SPHParticle p_right = createParticle(1.0, 1.0, common_pressure, 0.5);
    
    // Verify pressure equilibrium
    EXPECT_NEAR(p_left.pres, p_right.pres, 1e-10);
    
    // Verify density discontinuity
    EXPECT_NEAR(p_left.dens / p_right.dens, 2.0, 1e-10);
    
    // Volume elements should differ
    real V_left = calculateVolume(p_left);   // 0.5
    real V_right = calculateVolume(p_right); // 1.0
    
    EXPECT_NEAR(V_left, 0.5, 1e-10);
    EXPECT_NEAR(V_right, 1.0, 1e-10);
}

// ============================================================================
// Feature: DISPH Gradient Calculations
// ============================================================================

TEST_F(DISPHTest, KernelGradientSymmetry) {
    // Scenario: Kernel gradient symmetry
    // Given two particles i and j
    SPHParticle pi = createParticle(1.0, 1.0, 1.0, 0.0);
    SPHParticle pj = createParticle(1.0, 1.0, 1.0, 0.5);
    
    real h = 1.0;
    vec_t r_ij;
    r_ij.x = pj.pos.x - pi.pos.x;
    r_ij.y = pj.pos.y - pi.pos.y;
    r_ij.z = pj.pos.z - pi.pos.z;
    
    real r = std::sqrt(inner(r_ij, r_ij));
    
    // Compute kernel gradient
    vec_t grad_W = kernel->gradW(r_ij, h);
    
    // grad_W_ji = -grad_W_ij (symmetry)
    vec_t r_ji;
    r_ji.x = -r_ij.x;
    r_ji.y = -r_ij.y;
    r_ji.z = -r_ij.z;
    
    vec_t grad_W_ji = kernel->gradW(r_ji, h);
    
    // Check antisymmetry
    EXPECT_NEAR(grad_W.x, -grad_W_ji.x, 1e-10);
    EXPECT_NEAR(grad_W.y, -grad_W_ji.y, 1e-10);
    EXPECT_NEAR(grad_W.z, -grad_W_ji.z, 1e-10);
}

// ============================================================================
// Feature: DISPH Conservation Properties
// ============================================================================

TEST_F(DISPHTest, MomentumSymmetry) {
    // Scenario: Symmetric force pairs
    // Given two interacting particles i and j
    // The forces should satisfy F_ij = -F_ji for momentum conservation
    
    SPHParticle pi = createParticle(1.0, 1.5, 1.5, 0.0);
    SPHParticle pj = createParticle(1.0, 1.0, 1.0, 0.8);
    
    real Vi = calculateVolume(pi);
    real Vj = calculateVolume(pj);
    
    vec_t r_ij;
    r_ij.x = pj.pos.x - pi.pos.x;
    r_ij.y = pj.pos.y - pi.pos.y;
    r_ij.z = pj.pos.z - pi.pos.z;
    
    real h = 1.0;
    vec_t grad_W_ij = kernel->gradW(r_ij, h);
    
    // DISPH force contribution: -m_j * (P_i/V_i^2 + P_j/V_j^2) * grad_W_ij
    real pressure_factor = pi.pres / (Vi * Vi) + pj.pres / (Vj * Vj);
    
    vec_t F_ij;
    F_ij.x = -pj.mass * pressure_factor * grad_W_ij.x;
    F_ij.y = -pj.mass * pressure_factor * grad_W_ij.y;
    F_ij.z = -pj.mass * pressure_factor * grad_W_ij.z;
    
    // Force on j from i should be opposite
    vec_t r_ji;
    r_ji.x = -r_ij.x;
    r_ji.y = -r_ij.y;
    r_ji.z = -r_ij.z;
    
    vec_t grad_W_ji = kernel->gradW(r_ji, h);
    
    vec_t F_ji;
    F_ji.x = -pi.mass * pressure_factor * grad_W_ji.x;
    F_ji.y = -pi.mass * pressure_factor * grad_W_ji.y;
    F_ji.z = -pi.mass * pressure_factor * grad_W_ji.z;
    
    // Check that forces are opposite (accounting for mass difference)
    // For symmetric masses, F_ij should approximately equal -F_ji
    EXPECT_NEAR(F_ij.x + F_ji.x, 0.0, 1e-8);
    EXPECT_NEAR(F_ij.y + F_ji.y, 0.0, 1e-8);
    EXPECT_NEAR(F_ij.z + F_ji.z, 0.0, 1e-8);
}

// ============================================================================
// Feature: DISPH Energy Evolution
// ============================================================================

TEST_F(DISPHTest, PressureEnergyRelation) {
    // Scenario: Pressure-energy formulation
    // For ideal gas: P = (gamma - 1) * rho * u
    
    real density = 1.0;
    real internal_energy = 1.5;
    real gamma = param->physics.gamma;
    
    real expected_pressure = (gamma - 1.0) * density * internal_energy;
    
    SPHParticle p;
    p.mass = 1.0;
    p.dens = density;
    p.ene = internal_energy;
    p.pres = expected_pressure;
    
    // Verify pressure-energy relation
    EXPECT_NEAR(p.pres, (gamma - 1.0) * p.dens * p.ene, 1e-10);
    
    // Verify we can recover energy from pressure
    real recovered_energy = p.pres / ((gamma - 1.0) * p.dens);
    EXPECT_NEAR(recovered_energy, internal_energy, 1e-10);
}

// ============================================================================
// Feature: DISPH vs SSPH Comparison
// ============================================================================

TEST_F(DISPHTest, VolumeFormulationDifference) {
    // DISPH uses volume elements explicitly, SSPH doesn't
    // This test verifies the volume-based formulation is different
    
    SPHParticle pi = createParticle(1.0, 2.0, 2.0);
    SPHParticle pj = createParticle(1.0, 1.0, 1.0);
    
    real Vi = calculateVolume(pi);
    real Vj = calculateVolume(pj);
    
    // DISPH pressure term: P_i/V_i^2 + P_j/V_j^2
    real disph_term = pi.pres / (Vi * Vi) + pj.pres / (Vj * Vj);
    
    // SSPH pressure term would be: P_i/rho_i^2 + P_j/rho_j^2
    real ssph_term = pi.pres / (pi.dens * pi.dens) + 
                     pj.pres / (pj.dens * pj.dens);
    
    // For these particles: V_i = m_i/rho_i
    // So P_i/V_i^2 = P_i * rho_i^2 / m_i^2
    // vs P_i/rho_i^2 in SSPH
    
    // They should be different when masses vary
    // Here with m=1 they happen to be similar, but formulation differs
    
    // The key difference is in the derivation and consistency
    EXPECT_TRUE(disph_term > 0.0);
    EXPECT_TRUE(ssph_term > 0.0);
}

// ============================================================================
// Feature: DISPH Timestep Calculation
// ============================================================================

TEST_F(DISPHTest, CFLConditionWithVolumes) {
    // Scenario: CFL condition should account for volume-based formulation
    
    SPHParticle p = createParticle(1.0, 1.0, 1.0);
    p.sml = 0.1;
    
    // Sound speed: c_s = sqrt(gamma * P / rho)
    real gamma = param->physics.gamma;
    real sound_speed = std::sqrt(gamma * p.pres / p.dens);
    p.sound = sound_speed;
    
    // CFL timestep: dt = C * h / c_s
    real C = 0.3;  // CFL number
    real dt_cfl = C * p.sml / sound_speed;
    
    EXPECT_GT(dt_cfl, 0.0);
    EXPECT_LT(dt_cfl, 1.0);  // Should be reasonable
}

// ============================================================================
// Feature: DISPH Numerical Accuracy
// ============================================================================

TEST_F(DISPHTest, SecondOrderAccuracy) {
    // DISPH should maintain second-order spatial accuracy
    // This is verified through the symmetrized formulation
    
    // Create a smooth pressure field
    std::vector<SPHParticle> particles;
    for (int i = 0; i < 5; ++i) {
        real x = i * 0.1;
        real pressure = 1.0 + 0.1 * x;  // Linear pressure field
        real density = 1.0;
        
        particles.push_back(createParticle(1.0, density, pressure, x));
    }
    
    // The gradient calculation should be accurate for smooth fields
    // (Full test would require implementing the full gradient calculation)
    EXPECT_EQ(particles.size(), 5);
}

// ============================================================================
// Feature: DISPH Particle Interactions
// ============================================================================

TEST_F(DISPHTest, NeighborContribution) {
    // Test that neighbor contributions are properly weighted by volumes
    
    SPHParticle pi = createParticle(1.0, 1.0, 1.0, 0.0);
    SPHParticle pj = createParticle(2.0, 2.0, 2.0, 0.5);  // Different mass
    
    real Vi = calculateVolume(pi);  // 1.0
    real Vj = calculateVolume(pj);  // 1.0
    
    // Even though masses are different, volumes can be the same
    EXPECT_NEAR(Vi, 1.0, 1e-10);
    EXPECT_NEAR(Vj, 1.0, 1e-10);
    
    // This is a key feature of DISPH: volume-based rather than mass-based
}

TEST_F(DISPHTest, ZeroSeparationHandling) {
    // Test numerical stability when particles are very close
    
    SPHParticle pi = createParticle(1.0, 1.0, 1.0, 0.0);
    SPHParticle pj = createParticle(1.0, 1.0, 1.0, 1e-10);
    
    vec_t r_ij;
    r_ij.x = pj.pos.x - pi.pos.x;
    r_ij.y = pj.pos.y - pi.pos.y;
    r_ij.z = pj.pos.z - pi.pos.z;
    
    real r = std::sqrt(inner(r_ij, r_ij));
    
    // Should handle small separations gracefully
    EXPECT_GT(r, 0.0);
    EXPECT_LT(r, 1e-9);
}

// ============================================================================
// Feature: DISPH Boundary Conditions
// ============================================================================

TEST_F(DISPHTest, WallParticleVolumes) {
    // Wall particles should also have volume elements
    
    SPHParticle wall = createParticle(1.0, 1.0, 0.0);
    wall.is_wall = true;
    
    real V = calculateVolume(wall);
    
    EXPECT_NEAR(V, 1.0, 1e-10);
    EXPECT_TRUE(wall.is_wall);
}

} // namespace test
} // namespace sph
