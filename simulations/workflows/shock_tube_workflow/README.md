# Shock Tube Workflow

1D Sod shock tube problem comparing GDISPH and SSPH methods against analytical solution.

## Quick Start

```bash
cd 01_simulation

# Build plugin
mkdir -p build && cd build && cmake .. && make && cd ..

# Run GDISPH simulation
../../../../build/sph1d build/libshock_tube_plugin.dylib config/gdisph.json

# Run SSPH simulation  
../../../../build/sph1d build/libshock_tube_plugin.dylib config/ssph.json

# Compare results
python3 scripts/compare_methods.py
```

## Structure

```
01_simulation/
├── config/                        # Configuration files
│   ├── gdisph.json               # GDISPH method
│   ├── ssph.json                 # SSPH method
│   └── test.json                 # Quick test
├── src/
│   └── plugin.cpp                # Simulation setup
├── scripts/
│   └── compare_methods.py        # GDISPH vs SSPH comparison
├── results_gdisph/               # GDISPH outputs (gitignored)
├── results_ssph/                 # SSPH outputs (gitignored)
├── comparison_results/           # Comparison plots
└── CMakeLists.txt
```

## Physics

**Sod Shock Tube** - Classic 1D Riemann problem:
- Left state (x < 0): ρ=1.0, P=1.0, v=0
- Right state (x > 0): ρ=0.125, P=0.1, v=0
- Creates shock wave, contact discontinuity, and rarefaction wave
- Analytical solution available for comparison

## Configurations

- **config/gdisph.json** - GDISPH (Godunov DISPH) with Riemann solver
- **config/ssph.json** - SSPH (Standard SPH) density-energy formulation
- **config/test.json** - Quick test run

## Results

The comparison script generates:
- `comparison_results/shock_tube_comparison_timesteps.png` - Timestep-by-timestep comparison
- `comparison_results/shock_tube_error_evolution.png` - Error vs time
- `comparison_results/comparison_summary.txt` - Statistical summary

## See Also

- `/simulations/workflows/WORKFLOWS_GUIDE.md` - Complete workflow documentation
- `/analysis/README.md` - Analysis library documentation
