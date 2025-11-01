# DISPH Implementation Guide: TDD/BDD Approach

## Overview

This document describes the Test-Driven Development (TDD) and Behavior-Driven Development (BDD) approach for implementing DISPH (Density-Independent Smoothed Particle Hydrodynamics) based on the paper:

**"Novel Hydrodynamic Schemes Capturing Shocks and Contact Discontinuities"**  
by Takuhiro Yuasa & Masao Mori (2023) - arXiv:2312.03224

## Key DISPH Concepts from the Paper

### 1. Density-Independent Formulation

**Problem in Standard SPH (SSPH)**:
- Uses density-dependent formulation: `dv/dt = -sum_j m_j (P_i/ρ_i² + P_j/ρ_j²) ∇W`
- Creates spurious surface tension at contact discontinuities
- Leads to unphysical mixing and disrupted pressure equilibrium

**DISPH Solution**:
- Uses volume elements: `V_i = m_i / ρ_i`
- Momentum equation: `dv/dt = -sum_j m_j (P_i/V_i² + P_j/V_j²) ∇W`
- Eliminates spurious surface tension
- Maintains sharp contact discontinuities

### 2. Key Equations

**Volume Element**:
```
V_i = m_i / ρ_i
```

**Momentum (Pressure Force)**:
```
dv_i/dt = -Σ_j m_j (P_i/V_i² + P_j/V_j²) ∇_i W_ij
```

**Energy**:
```
du_i/dt = Σ_j m_j (P_i/V_i² + P_j/V_j²) v_ij · ∇_i W_ij
```

where `v_ij = v_i - v_j`

**Density** (still computed for EOS):
```
ρ_i = Σ_j m_j W_ij
```

**Equation of State** (ideal gas):
```
P = (γ - 1) ρ u
```

### 3. GDISPH (Godunov DISPH)

**GDISPH Case 1** (recommended):
- Integrates Riemann solver with DISPH volume formulation
- No artificial viscosity needed
- Best performance on shocks and contact discontinuities

**Key difference from DISPH**:
- DISPH uses artificial viscosity for shocks
- GDISPH uses Riemann solver (HLL, HLLC, etc.)

### 4. Standard Test Problems

From the paper, DISPH should be tested on:

1. **Sod Shock Tube** (1D Riemann problem)
   - Left: ρ=1.0, P=1.0, v=0
   - Right: ρ=0.125, P=0.1, v=0
   - Tests shock, contact discontinuity, rarefaction

2. **Pressure Equilibrium**
   - Two fluids: same pressure, different density
   - Tests spurious surface tension suppression

3. **Sedov-Taylor Blast Wave**
   - Point explosion in uniform medium
   - Tests self-similar solution, R ∝ t^(2/5)

4. **Kelvin-Helmholtz Instability**
   - Shear layer instability
   - Tests contact discontinuity + fluid instability

## BDD Scenarios

See `test/features/disph.feature` for complete Gherkin scenarios.

### Example Scenario: Volume Element Calculation

```gherkin
Feature: DISPH Volume Element Calculation
  
  Scenario: Computing particle volume elements
    Given a particle with mass 1.0 and density 2.0
    When I calculate the volume element
    Then the volume should be 0.5
    And the volume should equal mass divided by density
```

### Example Scenario: Pressure Equilibrium

```gherkin
Feature: DISPH Contact Discontinuity Handling

  Scenario: Maintaining sharp density interfaces
    Given a contact discontinuity with density ratio 2:1
    And equal pressure across the interface
    When I evolve the system for 10 timesteps
    Then the density interface should remain sharp
    And no artificial mixing should occur
    And pressure should remain equilibrated
```

## TDD Implementation Plan

### Phase 1: Core Volume Calculations (COMPLETED)

Tests in `test/unit_tests/test_disph.cpp`:

- ✅ Volume element calculation: `V = m/ρ`
- ✅ Volume updates with density changes
- ✅ Volume formulation differs from mass formulation

### Phase 2: Pressure Force Formulation (IN PROGRESS)

- [ ] Pressure gradient using volume elements
- [ ] Symmetric force pairs for momentum conservation
- [ ] Pressure equilibrium across contact discontinuities

### Phase 3: Integration Tests (PLANNED)

Tests in `test/unit_tests/test_disph_integration.cpp`:

- [ ] Sod shock tube evolution
- [ ] Pressure equilibrium maintenance
- [ ] Sedov-Taylor self-similarity
- [ ] Kelvin-Helmholtz instability growth

### Phase 4: Conservation Properties (PLANNED)

- [ ] Mass conservation (exact)
- [ ] Momentum conservation
- [ ] Energy conservation (< 1% drift)

### Phase 5: GDISPH Integration (FUTURE)

- [ ] Riemann solver interface
- [ ] Volume-weighted flux calculations
- [ ] Comparison with AV-based DISPH

## Code Structure

### Existing Implementation

The codebase already has DISPH infrastructure:

```
include/algorithms/disph/
├── d_pre_interaction.hpp    # Neighbor search, density
└── d_fluid_force.hpp         # Pressure forces

src/algorithms/disph/
├── d_pre_interaction.cpp
└── d_fluid_force.cpp
```

### Module Registration

DISPH modules use the factory pattern:

```cpp
REGISTER_MODULE("disph", "fluid_force", sph::disph::FluidForce)
REGISTER_MODULE("disph", "pre_interaction", sph::disph::PreInteraction)
```

### Key Classes

**SPHParticle** (`include/core/particle.hpp`):
```cpp
struct SPHParticle {
    vec_t pos, vel, acc;      // Kinematics
    real mass, dens, pres;    // Thermodynamics  
    real ene, dene;           // Energy
    real sml;                 // Smoothing length
    real sound;               // Sound speed
    // ... other fields
};
```

**vec_t** (`include/utilities/vector_type.hpp`):
```cpp
class vec_t {
    real vec[DIM];
    
    // Access via operator[]
    real& operator[](int i);
    
    // vec[0], vec[1], vec[2] for x, y, z
};
```

**SPHParameters** (`include/core/parameters.hpp`):
```cpp
struct SPHParameters {
    real gamma;              // Adiabatic index
    int neighbor_number;     // Target neighbors
    KernelType kernel;       // Kernel function
    
    struct Time { ... } time;
    struct ArtificialViscosity { ... } av;
    // ... other nested structs
};
```

## Implementation Guidelines

### 1. Volume Element Calculation

```cpp
// In d_pre_interaction.cpp
void PreInteraction::calculation(SPHParameters *param, Simulation *sim) {
    auto& particles = sim->get_particles();
    
    // Compute density first (standard SPH)
    compute_density(particles, kernel);
    
    // Compute volume elements
    for (auto& p : particles) {
        real volume = p.mass / p.dens;
        // Store or use directly in force calculation
    }
}
```

### 2. Pressure Force Calculation

```cpp
// In d_fluid_force.cpp  
void FluidForce::calculation(SPHParameters *param, Simulation *sim) {
    auto& particles = sim->get_particles();
    
    for (size_t i = 0; i < particles.size(); ++i) {
        auto& pi = particles[i];
        real Vi = pi.mass / pi.dens;
        
        for (int j : pi.neighbor_list) {
            auto& pj = particles[j];
            real Vj = pj.mass / pj.dens;
            
            vec_t rij = pi.pos - pj.pos;
            real r = std::abs(rij);
            vec_t gradW = kernel->dw(rij, r, pi.sml);
            
            // DISPH pressure term
            real pressure_term = pi.pres / (Vi * Vi) + pj.pres / (Vj * Vj);
            
            // Acceleration contribution
            vec_t acc_contrib = gradW * (-pj.mass * pressure_term);
            pi.acc += acc_contrib;
        }
    }
}
```

### 3. Energy Evolution

```cpp
// Also in d_fluid_force.cpp
void FluidForce::calculation(...) {
    // ... pressure force loop ...
    
    // Energy change rate
    vec_t vij = pi.vel - pj.vel;
    real energy_term = pj.mass * pressure_term * inner_product(vij, gradW);
    pi.dene += energy_term;
}
```

## Testing Strategy

### Unit Tests

Run specific tests:
```bash
cd build
./sph_unit_tests --gtest_filter=DISPHTest.*
./sph_unit_tests --gtest_filter=DISPHTest.VolumeElementBasicCalculation
```

### Integration Tests

Run full simulation tests:
```bash
./sph_unit_tests --gtest_filter=DISPHIntegrationTest.SodShockTube*
```

### Benchmark Comparisons

Compare DISPH vs SSPH vs GDISPH:
```bash
./build/sph1d sample/shock_tube/shock_tube_disph.json
./build/sph1d sample/shock_tube/shock_tube_ssph.json
./build/sph1d sample/shock_tube/shock_tube_gdisph.json
```

## Expected Results

### Pressure Equilibrium Test

**SSPH (bad)**:
- Spurious velocities develop at interface
- Density interface becomes diffuse
- Pressure deviates from equilibrium

**DISPH (good)**:
- No spurious velocities
- Sharp density interface maintained
- Pressure remains in equilibrium

### Sod Shock Tube

Both DISPH and GDISPH should:
- Capture shock accurately
- Preserve contact discontinuity sharpness
- Match analytical solution

**Advantage of GDISPH over DISPH**:
- No artificial viscosity parameters needed
- Less numerical diffusion
- Better shock resolution

### Kelvin-Helmholtz Instability

DISPH should show:
- Proper vortex development
- Realistic instability growth
- No excessive diffusion at density interface

SSPH would show:
- Suppressed instability growth (spurious surface tension)
- Excessive diffusion
- Unphysical behavior

## References

1. Yuasa & Mori (2023). "Novel Hydrodynamic Schemes Capturing Shocks and Contact Discontinuities". arXiv:2312.03224
   - https://arxiv.org/abs/2312.03224
   
2. Saitoh & Makino (2013). "A Density-Independent Formulation of Smoothed Particle Hydrodynamics". ApJ 768, 44
   - Original DISPH paper

3. Hopkins (2015). "A New Class of Accurate, Mesh-Free Hydrodynamic Simulation Methods". MNRAS 450, 53
   - MFM/MFV methods, related density-independent approaches

## Development Workflow

### TDD Cycle

1. **Red**: Write failing test
   ```bash
   # Test fails: function not implemented
   ```

2. **Green**: Implement minimal code to pass
   ```cpp
   // Implement just enough to pass test
   ```

3. **Refactor**: Improve code quality
   ```cpp
   // Clean up, optimize, document
   ```

4. **Repeat** for next feature

### BDD Cycle

1. **Feature**: Write Gherkin scenario
   ```gherkin
   Scenario: Volume element calculation
     Given a particle...
   ```

2. **Implement**: Create step definitions (tests)
   ```cpp
   TEST_F(DISPHTest, VolumeElementBasicCalculation) { ... }
   ```

3. **Code**: Implement actual algorithm
   ```cpp
   real volume = mass / density;
   ```

4. **Verify**: Run tests, compare with paper

## Current Status

### Completed
- ✅ BDD feature file created (`test/features/disph.feature`)
- ✅ TDD unit test scaffolding (`test/unit_tests/test_disph.cpp`)
- ✅ Integration test scaffolding (`test/unit_tests/test_disph_integration.cpp`)
- ✅ Understanding of codebase structure
- ✅ Tests integrated into CMake build system

### In Progress
- 🔄 Fixing tests to match actual codebase API
- 🔄 Implementing volume element calculations

### Next Steps
1. Fix test compilation errors (vector access, parameter structure)
2. Implement volume element storage/caching
3. Implement DISPH pressure force
4. Implement DISPH energy evolution
5. Run Sod shock tube test
6. Compare with SSPH and analytical solution
7. Document results

## Notes

- Tests should pass with DIM=1, 2, and 3
- Use `inner_product()` not `inner()` for dot product
- Access vector components via `vec[0]`, `vec[1]`, `vec[2]`
- Parameters accessed directly, not via nested `param->sph.type`
- Kernel functions return `vec_t` for gradients
