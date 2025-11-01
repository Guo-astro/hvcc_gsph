# 2D Sedov-Taylor Blast Wave (DISPH)

## Overview

This simulation implements the classic Sedov-Taylor blast wave problem in 2D using the **DISPH** (Density Independent SPH) formulation.

## Physics

The Sedov-Taylor blast wave is a fundamental hydrodynamics test problem:
- **Initial state**: Uniform medium with density ρ₀ = 1.0
- **Energy injection**: E₀ = 1.0 deposited in small central region
- **Evolution**: Circular shock wave expands self-similarly

The problem has an analytical solution that can be used for validation.

## Why DISPH?

This simulation uses **DISPH** instead of GSPH or SSPH because:

✅ **Better pressure handling**: Pressure-energy formulation is ideal for blast waves  
✅ **Conservation properties**: Superior energy and momentum conservation  
✅ **Simpler than GSPH**: No need for Riemann solver or reconstruction  
✅ **Good shock capturing**: Handles discontinuities well without excessive complexity  

Compared to the 3D version (which uses GSPH), DISPH provides:
- More stable evolution for symmetric problems
- Better force symmetry (important for circular/spherical shocks)
- Easier to analyze and validate

## Parameters

| Parameter | Value | Description |
|-----------|-------|-------------|
| Domain | [-0.5, 0.5]² | Square domain |
| Particles | 100×100 = 10,000 | Uniform Cartesian grid |
| γ | 1.4 | Adiabatic index (diatomic) |
| E₀ | 1.0 | Total explosion energy |
| ρ₀ | 1.0 | Background density |
| End time | 0.1 | When shock reaches ~r=0.3 |
| Output Δt | 0.01 | 10 snapshots |

## Building

```bash
cd simulations/sedov_taylor_2d
chmod +x build.sh
./build.sh
```

This creates `build/libsedov_taylor_2d_plugin.dylib` (macOS) or `.so` (Linux).

## Running

From the main `build/` directory:

```bash
./sph2d ../simulations/sedov_taylor_2d/build/libsedov_taylor_2d_plugin.dylib
```

Or use the convenience script:

```bash
./run_simulation sedov_taylor_2d
```

## Output Structure

```
simulations/sedov_taylor_2d/
├── run_YYYY-MM-DD_HHMMSS_DISPH_2d/
│   ├── metadata.json              # Run metadata
│   ├── config.json                # Configuration used
│   ├── initial_conditions.csv     # t=0 state
│   ├── source/
│   │   └── sedov_taylor_2d.cpp   # Source code snapshot
│   ├── outputs/
│   │   ├── csv/                   # CSV snapshots
│   │   │   ├── 00000.csv
│   │   │   ├── 00001.csv
│   │   │   └── ...
│   │   └── binary/                # Binary snapshots (.sph)
│   │       ├── 00000.sph
│   │       └── ...
│   ├── analysis/                  # Analysis results
│   ├── visualizations/            # Plots/animations
│   └── logs/                      # Simulation logs
└── latest -> run_*/               # Symlink to latest run
```

## Analytical Solution

See `analytical/sedov_taylor_2d_solution.py` for computing the exact solution.

The solution is self-similar with scaling:
- Shock radius: R(t) ∝ t^(2/5) in 2D
- Similarity variable: ξ = r / R(t)

Compare simulation vs analytical:

```bash
python analytical/sedov_taylor_2d_solution.py --run latest
```

## Validation

Key metrics to check:
1. **Shock position**: Should match R(t) = ξ₀(E₀/ρ₀)^(1/4) t^(2/5)
2. **Energy conservation**: Total energy should stay ≈ E₀
3. **Symmetry**: Shock should be perfectly circular
4. **Density jump**: Should match Rankine-Hugoniot conditions

## Visualization

Create plots:

```bash
# Radial profiles at different times
python ../../analysis/plotting.py latest --plot-type radial

# Animation of shock evolution
python ../../analysis/make_animation.py latest --output shock_evolution.mp4
```

## Comparison with 3D Version

| Feature | 2D (DISPH) | 3D (GSPH) |
|---------|-----------|-----------|
| **Method** | DISPH | GSPH |
| **Geometry** | Circular | Spherical |
| **Scaling** | R ∝ t^(2/5) | R ∝ t^(2/5) |
| **Particles** | 10,000 (100²) | 125,000 (50³) |
| **Neighbors** | 32 | 50 |
| **Gamma** | 1.4 | 5/3 |

## References

1. **Original work**:
   - Sedov (1959) - "Similarity and Dimensional Methods in Mechanics"
   - Taylor (1950) - "The Formation of a Blast Wave"

2. **DISPH method**:
   - Saitoh & Makino (2013) - "A density-independent formulation of smoothed particle hydrodynamics"
   - Hopkins (2013) - "A general class of Lagrangian smoothed particle hydrodynamics methods"

3. **Numerical solution**:
   - Kamm & Timmes (2007) - "On Efficient Generation of Numerically Robust Sedov Solutions"

## Notes

- The 2D solution differs from 3D only in geometric factors
- DISPH handles the strong shock better than standard SPH
- Circular symmetry makes this ideal for testing conservation
- The analytical solution assumes γ-law EOS and perfect symmetry

## Version History

- **1.0.0** (2025-01-11): Initial DISPH implementation
