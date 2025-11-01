# Benchmark Simulations

Standard test problems for validating SPH implementations. These simulations have well-known analytical or reference solutions that allow quantitative verification of the code.

## Subdirectories

### Shock Tubes (`shock_tubes/`)
Classic 1D Riemann problems testing shock capturing and contact discontinuities.

**Simulations:**
- `sod_shock_tube.cpp` - Standard Sod problem (ρ_L=1.0, ρ_R=0.125, p_L=1.0, p_R=0.1)
- `strong_shock.cpp` - High compression ratio shock test
- `with_heating_cooling.cpp` - Shock tube with thermal processes
- `astro_units.cpp` - Shock in astrophysical units
- `2d_shock_tube.cpp` - 2D extension of Sod shock
- `2p5d_*.cpp` - 2.5D shock tube variants (thin slices with vertical structure)

**Key Tests:**
- Shock capturing accuracy
- Contact discontinuity resolution  
- Conservation properties
- Artificial viscosity behavior

### Sedov-Taylor (`sedov_taylor/`)
Self-similar blast wave solution for point explosion.

**Tests:**
- Spherical symmetry preservation
- Shock front evolution
- Energy conservation
- Convergence with resolution

### Gresho-Chan Vortex (`gresho_vortex/`)
2D vortex stability test for advection and pressure gradients.

**Tests:**
- Vortex stability over time
- Pressure gradient handling
- Angular momentum conservation
- Absence of spurious diffusion

### Evrard Collapse (`evrard_collapse/`)
Spherical collapse test for gravity + hydrodynamics coupling.

**Tests:**
- Gravitational collapse dynamics
- Bounce and shock formation
- Energy conservation (grav + hydro)
- Virial equilibrium

## Usage

All benchmarks should be run with their corresponding configuration files in `configs/benchmarks/`.

Example:
```bash
./sph sod_shock_tube configs/benchmarks/shock_tubes/sod.json 8
```

## Expected Outputs

Each benchmark should generate:
- `output_*.txt` - Particle snapshots
- `energy.txt` - Energy history
- Conservation analysis showing max errors < 0.1%

## Adding New Benchmarks

1. Create subdirectory: `src/samples/benchmarks/my_test/`
2. Implement `my_test.cpp` with initial conditions
3. Add `REGISTER_SAMPLE("my_test", load_my_test)`
4. Create config: `configs/benchmarks/my_test.json`
5. Document expected results and validation criteria
6. Add test to CI/regression suite

## References

- Sod Shock Tube: Sod (1978) JCP
- Sedov-Taylor: Sedov (1959)
- Gresho Vortex: Gresho & Chan (1990)
- Evrard Collapse: Evrard (1988)
