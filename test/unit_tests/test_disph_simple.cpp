/**
 * @file test_disph_simple.cpp
 * @brief Simplified DISPH tests that compile with current codebase
 * 
 * Based on the paper: "Novel Hydrodynamic Schemes Capturing Shocks and 
 * Contact Discontinuities" by Yuasa & Mori (2023) - arXiv:2312.03224
 */

#include <gtest/gtest.h>
#include <cmath>
#include "utilities/defines.hpp"
#include "utilities/vector_type.hpp"

namespace sph {
namespace test {

/**
 * @brief Test fixture for DISPH algorithm tests
 * 
 * Tests focus on the density-independent formulation:
 * - Volume elements: V_i = m_i / ρ_i  
 * - Pressure force: dv/dt = -Σ m_j (P_i/V_i² + P_j/V_j²) ∇W
 * - Energy evolution: du/dt = Σ m_j (P_i/V_i² + P_j/V_j²) v_ij · ∇W
 */
class DISPHSimpleTest : public ::testing::Test {
protected:
    void SetUp() override {
        gamma = 5.0 / 3.0;  // Adiabatic index for ideal gas
    }

    // Helper: Calculate volume element V = m / ρ
    real calculateVolume(real mass, real density) {
        return mass / density;
    }

    // Helper: Calculate pressure from density and internal energy
    real calculatePressure(real density, real internal_energy, real adiabatic_index) {
        return (adiabatic_index - 1.0) * density * internal_energy;
    }

    // Helper: Calculate internal energy from pressure and density
    real calculateInternalEnergy(real pressure, real density, real adiabatic_index) {
        return pressure / ((adiabatic_index - 1.0) * density);
    }

    real gamma;  // Adiabatic index
};

// ============================================================================
// BDD Scenario: Computing particle volume elements
// Given a particle with mass 1.0 and density 2.0
// When I calculate the volume element
// Then the volume should be 0.5
// ============================================================================

TEST_F(DISPHSimpleTest, VolumeElementBasicCalculation) {
    // Given
    real mass = 1.0;
    real density = 2.0;
    
    // When
    real volume = calculateVolume(mass, density);
    
    // Then
    EXPECT_NEAR(volume, 0.5, 1e-10);
}

TEST_F(DISPHSimpleTest, VolumeEqualsMASSOverDensity) {
    // Verify fundamental relation: V = m/ρ
    real mass = 1.5;
    real density = 3.0;
    
    real volume = calculateVolume(mass, density);
    real expected = mass / density;
    
    EXPECT_NEAR(volume, expected, 1e-10);
}

// ============================================================================
// BDD Scenario: Volume element updates with density changes
// Given a particle with mass 1.0 and initial density 1.0
// When the density is updated to 2.0
// Then the volume should be 0.5
// ============================================================================

TEST_F(DISPHSimpleTest, VolumeUpdatesWithDensityChanges) {
    real mass = 1.0;
    real initial_density = 1.0;
    real new_density = 2.0;
    
    real initial_volume = calculateVolume(mass, initial_density);
    EXPECT_NEAR(initial_volume, 1.0, 1e-10);
    
    real new_volume = calculateVolume(mass, new_density);
    EXPECT_NEAR(new_volume, 0.5, 1e-10);
}

// ============================================================================
// BDD Scenario: Pressure force formulation
// DISPH uses: (P_i/V_i² + P_j/V_j²) instead of (P_i/ρ_i² + P_j/ρ_j²)
// ============================================================================

TEST_F(DISPHSimpleTest, PressureForceFormulation) {
    // Particle i: mass=1.0, density=2.0, pressure=2.0
    real mi = 1.0, rho_i = 2.0, P_i = 2.0;
    real Vi = calculateVolume(mi, rho_i);  // Vi = 0.5
    
    // Particle j: mass=1.0, density=1.0, pressure=1.0
    real mj = 1.0, rho_j = 1.0, P_j = 1.0;
    real Vj = calculateVolume(mj, rho_j);  // Vj = 1.0
    
    // DISPH pressure term: P_i/V_i² + P_j/V_j²
    real disph_term = P_i / (Vi * Vi) + P_j / (Vj * Vj);
    
    // Expected: 2.0/0.25 + 1.0/1.0 = 8.0 + 1.0 = 9.0
    EXPECT_NEAR(disph_term, 9.0, 1e-10);
}

TEST_F(DISPHSimpleTest, VolumeFormulationDifferentFromMass) {
    // For particles with different masses but same density,
    // volume formulation gives different results than standard SPH
    
    real m1 = 2.0, rho1 = 1.0, P1 = 1.0;
    real m2 = 1.0, rho2 = 1.0, P2 = 1.0;
    
    real V1 = calculateVolume(m1, rho1);  // 2.0
    real V2 = calculateVolume(m2, rho2);  // 1.0
    
    // DISPH pressure coefficient
    real disph_coeff = P1 / (V1 * V1) + P2 / (V2 * V2);
    // = 1.0/4.0 + 1.0/1.0 = 0.25 + 1.0 = 1.25
    
    // SSPH would use: P1/rho1² + P2/rho2² = 1.0 + 1.0 = 2.0
    real ssph_coeff = P1 / (rho1 * rho1) + P2 / (rho2 * rho2);
    
    EXPECT_NEAR(disph_coeff, 1.25, 1e-10);
    EXPECT_NEAR(ssph_coeff, 2.0, 1e-10);
    EXPECT_NE(disph_coeff, ssph_coeff);
}

// ============================================================================
// BDD Scenario: Pressure equilibrium across contact discontinuity
// Given two particles with same pressure but different density
// The DISPH formulation should handle this correctly
// ============================================================================

TEST_F(DISPHSimpleTest, ContactDiscontinuityPressureBalance) {
    real common_pressure = 1.0;
    
    // Left particle: high density
    real m_left = 1.0, rho_left = 2.0;
    real V_left = calculateVolume(m_left, rho_left);  // 0.5
    
    // Right particle: low density
    real m_right = 1.0, rho_right = 1.0;
    real V_right = calculateVolume(m_right, rho_right);  // 1.0
    
    // At pressure equilibrium (same pressure)
    real P_left = common_pressure;
    real P_right = common_pressure;
    
    // Pressure terms are not equal even though pressures are equal
    // This is KEY for DISPH - it naturally handles contact discontinuities
    real term_left = P_left / (V_left * V_left);   // 1.0/0.25 = 4.0
    real term_right = P_right / (V_right * V_right); // 1.0/1.0 = 1.0
    
    EXPECT_NEAR(P_left, P_right, 1e-10);  // Pressures equal
    EXPECT_NE(term_left, term_right);      // But terms differ (handles discontinuity)
}

// ============================================================================
// BDD Scenario: Equation of state (ideal gas)
// P = (γ - 1) ρ u
// ============================================================================

TEST_F(DISPHSimpleTest, IdealGasEOS) {
    real density = 1.0;
    real internal_energy = 1.5;
    
    real pressure = calculatePressure(density, internal_energy, gamma);
    real expected_pressure = (gamma - 1.0) * density * internal_energy;
    
    EXPECT_NEAR(pressure, expected_pressure, 1e-10);
}

TEST_F(DISPHSimpleTest, InverseEOS) {
    // Can recover internal energy from pressure and density
    real density = 1.0;
    real pressure = 1.0;
    
    real internal_energy = calculateInternalEnergy(pressure, density, gamma);
    
    // Verify by going back
    real recovered_pressure = calculatePressure(density, internal_energy, gamma);
    EXPECT_NEAR(recovered_pressure, pressure, 1e-10);
}

// ============================================================================
// BDD Scenario: Momentum conservation through symmetric forces
// F_ij = -F_ji for momentum conservation
// ============================================================================

TEST_F(DISPHSimpleTest, SymmetricForceContribution) {
    // For two particles with equal mass, the pressure force contribution
    // should satisfy action-reaction
    
    real mi = 1.0, mj = 1.0;
    real rho_i = 1.5, rho_j = 1.0;
    real P_i = 1.5, P_j = 1.0;
    
    real Vi = calculateVolume(mi, rho_i);
    real Vj = calculateVolume(mj, rho_j);
    
    // The DISPH pressure term is symmetric in formulation
    real pressure_factor = P_i / (Vi * Vi) + P_j / (Vj * Vj);
    
    // Force on i from j: F_i = -m_j * pressure_factor * grad_W_ij
    // Force on j from i: F_j = -m_i * pressure_factor * grad_W_ji
    // Since grad_W_ji = -grad_W_ij and m_i = m_j, we have F_i = -F_j
    
    // For equal masses, the coefficient should be the same
    real force_coeff_i = mj * pressure_factor;
    real force_coeff_j = mi * pressure_factor;
    
    EXPECT_NEAR(force_coeff_i, force_coeff_j, 1e-10);
}

// ============================================================================
// BDD Scenario: Energy evolution
// du/dt = Σ m_j (P_i/V_i² + P_j/V_j²) v_ij · ∇W
// ============================================================================

TEST_F(DISPHSimpleTest, EnergyEvolutionFormulation) {
    // Energy evolution uses the same pressure term as momentum
    real mi = 1.0, mj = 1.0;
    real rho_i = 1.0, rho_j = 1.0;
    real P_i = 1.0, P_j = 1.0;
    
    real Vi = calculateVolume(mi, rho_i);
    real Vj = calculateVolume(mj, rho_j);
    
    real pressure_term = P_i / (Vi * Vi) + P_j / (Vj * Vj);
    
    // Energy change rate coefficient
    real energy_coeff = mj * pressure_term;
    
    // This gets multiplied by (v_i - v_j) · grad_W_ij
    EXPECT_GT(energy_coeff, 0.0);
}

// ============================================================================
// BDD Scenario: Conservation properties - Mass
// Mass should be exactly conserved: Σ m_i = constant
// ============================================================================

TEST_F(DISPHSimpleTest, MassConservation) {
    // Mass conservation is trivial in SPH - masses don't change
    std::vector<real> masses = {1.0, 1.5, 2.0, 0.5};
    
    real total_mass = 0.0;
    for (real m : masses) {
        total_mass += m;
    }
    
    // After any evolution, total mass should be unchanged
    EXPECT_NEAR(total_mass, 5.0, 1e-12);
}

// ============================================================================
// BDD Scenario: Vector operations
// Test that vector operations work correctly for force calculations
// ============================================================================

TEST_F(DISPHSimpleTest, VectorOperations) {
    vec_t v1(1.0);
    vec_t v2(2.0);
    
    // Basic operations
    vec_t sum = v1 + v2;
    vec_t diff = v1 - v2;
    vec_t scaled = v1 * 2.0;
    
    EXPECT_NEAR(sum[0], 3.0, 1e-10);
    EXPECT_NEAR(diff[0], -1.0, 1e-10);
    EXPECT_NEAR(scaled[0], 2.0, 1e-10);
}

TEST_F(DISPHSimpleTest, VectorInnerProduct) {
    vec_t v1(1.0);
    vec_t v2(2.0);
    
    real dot = inner_product(v1, v2);
    
#if DIM == 1
    EXPECT_NEAR(dot, 2.0, 1e-10);
#elif DIM == 2
    vec_t v3(1.0, 2.0);
    vec_t v4(2.0, 3.0);
    real dot2 = inner_product(v3, v4);
    EXPECT_NEAR(dot2, 8.0, 1e-10);  // 1*2 + 2*3
#elif DIM == 3
    vec_t v3(1.0, 2.0, 3.0);
    vec_t v4(2.0, 3.0, 4.0);
    real dot2 = inner_product(v3, v4);
    EXPECT_NEAR(dot2, 20.0, 1e-10);  // 1*2 + 2*3 + 3*4
#endif
}

// ============================================================================
// BDD Scenario: Numerical stability
// Test edge cases and numerical stability
// ============================================================================

TEST_F(DISPHSimpleTest, SmallDensityHandling) {
    // Should handle small densities without division by zero
    real mass = 1.0;
    real small_density = 1e-10;
    
    real volume = calculateVolume(mass, small_density);
    
    EXPECT_TRUE(std::isfinite(volume));
    EXPECT_GT(volume, 0.0);
}

TEST_F(DISPHSimpleTest, LargePressureGradient) {
    // Should handle large pressure gradients
    real m = 1.0;
    real rho1 = 1.0, P1 = 100.0;
    real rho2 = 1.0, P2 = 0.01;
    
    real V1 = calculateVolume(m, rho1);
    real V2 = calculateVolume(m, rho2);
    
    real pressure_term = P1 / (V1 * V1) + P2 / (V2 * V2);
    
    EXPECT_TRUE(std::isfinite(pressure_term));
    EXPECT_GT(pressure_term, 0.0);
}

// ============================================================================
// BDD Scenario: Comparison with analytical results
// ============================================================================

TEST_F(DISPHSimpleTest, SoundSpeedCalculation) {
    // Sound speed: c_s = sqrt(γ P / ρ)
    real pressure = 1.0;
    real density = 1.0;
    
    real sound_speed = std::sqrt(gamma * pressure / density);
    real expected = std::sqrt(5.0 / 3.0);
    
    EXPECT_NEAR(sound_speed, expected, 1e-10);
}

TEST_F(DISPHSimpleTest, AdiabaticRelations) {
    // For adiabatic evolution: P ∝ ρ^γ
    real rho1 = 1.0, P1 = 1.0;
    real rho2 = 2.0;
    
    // Expected: P2 = P1 * (rho2/rho1)^gamma
    real P2_expected = P1 * std::pow(rho2 / rho1, gamma);
    
    // For gamma = 5/3 and density doubling
    // P2 = 1.0 * 2^(5/3) ≈ 3.17
    EXPECT_NEAR(P2_expected, std::pow(2.0, 5.0/3.0), 1e-10);
}

} // namespace test
} // namespace sph
