# Sedov-Taylor 2D Blast Wave - GDISPH Variant# 2D Sedov-Taylor Blast Wave (DISPH)



## Overview## Overview



This is the **GDISPH variant** of the 2D Sedov-Taylor blast wave simulation, created for **algorithm comparison** with the standard DISPH version in `sedov_taylor_2d/`.This simulation implements the classic Sedov-Taylor blast wave problem in 2D using the **DISPH** (Density Independent SPH) formulation.



**Purpose**: Compare GDISPH (Godunov DISPH) and DISPH shock capturing performance on the same problem.## Physics



## What is GDISPH?The Sedov-Taylor blast wave is a fundamental hydrodynamics test problem:

- **Initial state**: Uniform medium with density ρ₀ = 1.0

GDISPH combines:- **Energy injection**: E₀ = 1.0 deposited in small central region

- **DISPH formulation**: Density-independent pressure-energy formulation- **Evolution**: Circular shock wave expands self-similarly

- **Godunov method**: Riemann solver for computing fluxes between particles

- **Better shock capturing**: Improved handling of strong discontinuitiesThe problem has an analytical solution that can be used for validation.



Compared to standard DISPH:## Why DISPH?

- ✅ **Pros**: Sharper shocks, less numerical diffusion, better conservation

- ⚠️ **Cons**: More computational cost (Riemann solver), may need smaller timestepsThis simulation uses **DISPH** instead of GSPH or SSPH because:



## Physics✅ **Better pressure handling**: Pressure-energy formulation is ideal for blast waves  

✅ **Conservation properties**: Superior energy and momentum conservation  

Same setup as `sedov_taylor_2d/`:✅ **Simpler than GSPH**: No need for Riemann solver or reconstruction  

- **Dimension**: 2D✅ **Good shock capturing**: Handles discontinuities well without excessive complexity  

- **Initial conditions**: 

  - Uniform density ρ = 1.0Compared to the 3D version (which uses GSPH), DISPH provides:

  - Energy E₀ = 1.0 injected at origin- More stable evolution for symmetric problems

  - Zero initial velocity- Better force symmetry (important for circular/spherical shocks)

- **EOS**: Ideal gas (γ = 1.4)- Easier to analyze and validate

- **Domain**: [-0.5, 0.5]² (non-periodic)

## Parameters

## Building

| Parameter | Value | Description |

```bash|-----------|-------|-------------|

cd /path/to/sphcode| Domain | [-0.5, 0.5]² | Square domain |

./simulations/sedov_taylor_2d_gdisph/build.sh| Particles | 100×100 = 10,000 | Uniform Cartesian grid |

```| γ | 1.4 | Adiabatic index (diatomic) |

| E₀ | 1.0 | Total explosion energy |

## Running| ρ₀ | 1.0 | Background density |

| End time | 0.1 | When shock reaches ~r=0.3 |

```bash| Output Δt | 0.01 | 10 snapshots |

cd build

./sph2d ../simulations/sedov_taylor_2d_gdisph/config.json## Building

```

```bash

## Comparison with DISPHcd simulations/sedov_taylor_2d

chmod +x build.sh

To compare both algorithms:./build.sh

```

```bash

# Run both versionsThis creates `build/libsedov_taylor_2d_plugin.dylib` (macOS) or `.so` (Linux).

./sph2d ../simulations/sedov_taylor_2d/config.json

./sph2d ../simulations/sedov_taylor_2d_gdisph/config.json## Running

```

From the main `build/` directory:

Then analyze results side-by-side to compare:

- Shock sharpness```bash

- Numerical diffusion./sph2d ../simulations/sedov_taylor_2d/build/libsedov_taylor_2d_plugin.dylib

- Conservation properties```

- Computation time

Or use the convenience script:

See full README for detailed comparison methodology.

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
