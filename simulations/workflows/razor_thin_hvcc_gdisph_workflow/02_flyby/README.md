# 02_flyby - Stellar Flyby Simulation

High-velocity cloud collision simulation using GDISPH (Godunov SPH) to model cloud-cloud interactions.

## Directory Structure

```
02_flyby/
├── README.md                      # This file
├── CMakeLists.txt                 # Build configuration
├── .gitignore                     # Ignore build/output
│
├── config/
│   └── production.json           # Flyby simulation parameters
│
├── src/
│   └── plugin.cpp                # Flyby initialization plugin
│
├── scripts/
│   └── build.sh                  # Build helper script
│
├── data/                         # Input data (if any)
├── docs/                         # Documentation
├── results/                      # Post-processed outputs
│   ├── animations/
│   ├── plots/
│   └── analysis/
├── output/                       # Simulation data (gitignored)
└── build/                        # Build artifacts (gitignored)
```

## Quick Start

```bash
# Build the plugin
cd /Users/guo/OSS/sphcode/simulations/workflows/razor_thin_hvcc_gdisph_workflow
make build-flyby

# Run simulation
make run-flyby

# Visualize results
make visualize-flyby
```

## Configuration

- **production.json**: Full flyby simulation with GDISPH

## Physics

This step simulates a high-velocity stellar flyby through a gaseous disk, modeling:
- Cloud-cloud collision dynamics
- Shock formation and propagation
- GDISPH Riemann solver for shock capturing
- Conservation of mass, momentum, and energy

## Output

- **Simulation data**: `output/flyby/`
- **Results**: `results/plots/`, `results/animations/`

## Notes

- Uses GDISPH (Godunov SPH) for accurate shock capturing
- Initial conditions from 01_relaxation output
