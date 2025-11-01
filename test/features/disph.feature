# Feature: DISPH (Density-Independent SPH) Algorithm
#
# As an astrophysicist
# I want to use DISPH for simulations with contact discontinuities
# So that I can accurately model shocks and preserve pressure equilibrium
#
# Background:
#   DISPH addresses limitations in Standard SPH (SSPH) by using a density-independent
#   formulation that eliminates unphysical surface tension at contact discontinuities.
#   This is based on the paper "Novel Hydrodynamic Schemes Capturing Shocks and Contact
#   Discontinuities" by Yuasa & Mori (2023).

Feature: DISPH Volume Element Calculation
  
  Scenario: Computing particle volume elements
    Given a particle with mass 1.0 and density 2.0
    When I calculate the volume element
    Then the volume should be 0.5
    And the volume should equal mass divided by density

  Scenario: Volume element updates with density changes
    Given a particle with mass 1.0 and initial density 1.0
    When the density is updated to 2.0
    And I recalculate the volume element
    Then the volume should be 0.5

Feature: DISPH Pressure Force Calculation

  Scenario: Symmetric pressure force between equal pressure particles
    Given two particles at distance 1.0 with equal pressure 1.0
    When I calculate the DISPH pressure force
    Then the force should be nearly zero
    And pressure equilibrium should be maintained

  Scenario: Pressure force across contact discontinuity
    Given particle A with pressure 2.0 and density 2.0
    And particle B with pressure 2.0 and density 1.0
    When I calculate the DISPH pressure force
    Then the force should preserve pressure equilibrium
    And there should be no spurious surface tension

  Scenario: Volume-based pressure gradient
    Given two particles with different pressures
    When I calculate the pressure gradient using volume elements
    Then the gradient should use the DISPH formulation
    And the result should be different from standard SPH

Feature: DISPH Contact Discontinuity Handling

  Scenario: Maintaining sharp density interfaces
    Given a contact discontinuity with density ratio 2:1
    And equal pressure across the interface
    When I evolve the system for 10 timesteps
    Then the density interface should remain sharp
    And no artificial mixing should occur
    And pressure should remain equilibrated

  Scenario: No spurious velocities at interfaces
    Given particles at rest with a density discontinuity
    And pressure equilibrium
    When I calculate forces using DISPH
    Then particles should remain stationary
    And no unphysical repulsion should occur

Feature: DISPH Equation of Motion

  Scenario: Acceleration calculation with volume formulation
    Given a particle in a pressure gradient
    When I calculate acceleration using DISPH
    Then the force should use volume elements V_i and V_j
    And the formula should be: -sum(m_j * (P_i/V_i^2 + P_j/V_j^2) * grad_W)
    And the result should conserve momentum

  Scenario: Symmetric force pairs
    Given two interacting particles i and j
    When I calculate forces F_ij and F_ji
    Then F_ij should equal negative F_ji
    And momentum should be conserved

Feature: DISPH Energy Evolution

  Scenario: Pressure-energy formulation
    Given particles evolving under DISPH
    When I update internal energies
    Then energy should use pressure-based formulation
    And energy should be conserved in isolated system

  Scenario: Shock heating
    Given a shock tube problem
    When I evolve with DISPH
    Then post-shock energy should match Rankine-Hugoniot relations
    And energy conservation should be maintained

Feature: DISPH Gradient Calculations

  Scenario: Pressure gradient with volume elements
    Given a smooth pressure field
    When I calculate grad(P) using DISPH
    Then the gradient should use symmetrized volume formulation
    And the result should be second-order accurate

  Scenario: Kernel gradient symmetry
    Given two particles i and j
    When I compute grad_W_ij and grad_W_ji
    Then they should satisfy symmetry relations
    And volume-weighted gradients should be consistent

Feature: DISPH Integration with Riemann Solver (GDISPH)

  Scenario: GDISPH Case 1 - Volume-based Riemann solver
    Given a shock tube with strong discontinuity
    When I solve using GDISPH Case 1
    Then the Riemann solver should use volume elements
    And no artificial viscosity should be required
    And shocks should be captured accurately

  Scenario: GDISPH vs DISPH comparison
    Given identical initial conditions
    When I run DISPH with artificial viscosity
    And I run GDISPH Case 1 without artificial viscosity
    Then both should capture shocks
    But GDISPH should have less numerical diffusion

Feature: DISPH Artificial Viscosity (Optional)

  Scenario: Traditional DISPH with artificial viscosity
    Given a shock tube problem
    When I use DISPH with Monaghan AV
    Then shocks should be captured
    And AV parameters should control shock width

  Scenario: DISPH without artificial viscosity
    Given GDISPH formulation
    When I disable artificial viscosity
    Then shocks should still be captured via Riemann solver
    And contact discontinuities should be preserved

Feature: DISPH Timestep Calculation

  Scenario: CFL condition with volume elements
    Given particles with various volume elements
    When I calculate the timestep
    Then CFL should account for volume-based formulation
    And timestep should be stable

Feature: DISPH Neighbor Search

  Scenario: Smoothing length iteration
    Given a particle with initial smoothing length
    When I iterate to find neighbors using DISPH
    Then the number of neighbors should converge
    And smoothing length should adapt to local resolution

Feature: DISPH Conservation Properties

  Scenario: Mass conservation
    Given an isolated DISPH simulation
    When I evolve for multiple timesteps
    Then total mass should be conserved exactly

  Scenario: Momentum conservation
    Given an isolated DISPH simulation with no external forces
    When I evolve for multiple timesteps
    Then total momentum should be conserved
    And center of mass velocity should remain constant

  Scenario: Energy conservation
    Given an isolated adiabatic DISPH simulation
    When I evolve for multiple timesteps
    Then total energy (kinetic + internal) should be conserved
    And energy drift should be minimal

Feature: DISPH Standard Test Problems

  Scenario: Sod Shock Tube
    Given Sod shock tube initial conditions
    When I evolve using DISPH
    Then density, velocity, and pressure profiles should match analytical solution
    And shock position should be accurate
    And contact discontinuity should be sharp

  Scenario: Pressure Equilibrium Test
    Given two fluids with different densities but equal pressure
    When I evolve using DISPH
    Then pressure should remain equal
    And no spurious velocities should develop
    And density interface should remain sharp

  Scenario: Sedov-Taylor Blast Wave
    Given a point explosion in uniform medium
    When I evolve using DISPH
    Then shock radius should follow R ∝ t^(2/5)
    And self-similar solution should be matched

  Scenario: Kelvin-Helmholtz Instability
    Given a shear layer with density contrast
    When I evolve using DISPH
    Then vortices should develop
    And instability growth should match theory
    And no excessive diffusion should occur

Feature: DISPH vs SSPH Comparison

  Scenario: Contact discontinuity sharpness
    Given identical initial conditions
    When I run SSPH and DISPH
    Then DISPH should maintain sharper density interfaces
    And SSPH should show spurious mixing

  Scenario: Pressure equilibrium maintenance
    Given a two-phase medium at pressure equilibrium
    When I run SSPH and DISPH
    Then DISPH should maintain equilibrium better
    And SSPH should develop spurious velocities
