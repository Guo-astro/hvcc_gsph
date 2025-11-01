# Kelvin-Helmholtz Instability

## Overview

Classic 2D hydrodynamic instability test demonstrating shear flow instability. The simulation shows how velocity shear between two fluid layers becomes unstable and develops characteristic vortex rolls.

## Physics

- **Dimension**: 2D
- **SPH Method**: GSPH (Godunov SPH)
- **EOS**: Ideal gas (γ = 5/3)
- **Initial Conditions**:
  - **Top/bottom regions** (0 < y < 0.25, 0.75 < y < 1.0):
    - Density: ρ = 1.0
    - Velocity: v_x = -0.5, v_y = δv(x,y)
  - **Center region** (0.25 < y < 0.75):
    - Density: ρ = 2.0
    - Velocity: v_x = +0.5, v_y = δv(x,y)
  - Uniform pressure: P = 2.5
  - Velocity perturbation: δv_y = 0.1 sin(4πx) [exp(-((y-0.25)/σ)²) + exp(-((y-0.75)/σ)²)]
    with σ = 0.05

## Configuration

Default parameters (`config.json`):
- Resolution: 50×50 base grid with variable spacing (≈1875 particles)
- Domain: [0, 1]² with periodic boundaries
- Time: t = 0 → 10.0
- Kernel: Cubic spline
- SPH variant: GSPH (2nd order)
- Artificial viscosity: α = 1.0 with Balsara switch

## Expected Results

The Kelvin-Helmholtz instability develops in stages:
1. **Early (t < 1)**: Linear growth of sinusoidal perturbation
2. **Intermediate (1 < t < 3)**: Nonlinear development of vortex rolls
3. **Late (t > 3)**: Turbulent mixing and vortex pairing

Key features:
- Formation of 4 primary vortices (from 4-wavelength perturbation)
- Shear layer rollup and vortex core formation
- Secondary instabilities and turbulent mixing
- Conservation of momentum and energy

This tests the ability of the SPH method to:
1. Handle shear flows and contact discontinuities
2. Capture instability growth
3. Preserve vortical structures
4. Avoid excessive numerical diffusion

## Building

```bash
cd /path/to/sphcode
./simulations/kelvin_helmholtz/build.sh
```

Or rebuild from project root:
```bash
cd build && cmake --build . --target kelvin_helmholtz_plugin
```

## Running

```bash
./build/sph2d simulations/kelvin_helmholtz/build/kelvin_helmholtz_plugin.dylib
```

With config file:
```bash
./build/sph2d simulations/kelvin_helmholtz/config.json
```

## Output

- Binary files: `simulations/kelvin_helmholtz/output/{run_id}/sph_*.sph`
- Metadata: `simulations/kelvin_helmholtz/output/{run_id}/metadata.json`
- Source snapshot: `simulations/kelvin_helmholtz/output/{run_id}/source/`

## Analysis

Visualize vortex formation:

```python
from analysis.readers import read_simulation
import matplotlib.pyplot as plt
import numpy as np

# Load data
data = read_simulation('simulations/kelvin_helmholtz/output/{run_id}')

# Create animation of density evolution
fig, ax = plt.subplots(figsize=(8, 8))

for i, snapshot in enumerate(data):
    if i % 10 == 0:  # Every 10th frame
        ax.clear()
        sc = ax.scatter(snapshot['x'], snapshot['y'], 
                       c=snapshot['rho'], s=5, vmin=1, vmax=2)
        ax.set_xlim(0, 1)
        ax.set_ylim(0, 1)
        ax.set_aspect('equal')
        ax.set_title(f't = {snapshot["time"]:.2f}')
        plt.colorbar(sc, ax=ax, label='Density')
        plt.pause(0.1)

plt.show()
```

Measure vorticity:
```python
# Compute vorticity ω = ∂v_y/∂x - ∂v_x/∂y
from scipy.spatial import cKDTree

def compute_vorticity(snapshot):
    pos = np.column_stack([snapshot['x'], snapshot['y']])
    tree = cKDTree(pos)
    
    vorticity = []
    for i, p in enumerate(pos):
        neighbors = tree.query_ball_point(p, r=0.05)
        # Compute gradients using SPH kernel weighted average
        # ... (implementation details)
    
    return np.array(vorticity)
```

## Physics References

- **Kelvin-Helmholtz Instability**: Occurs when velocity shear exists across interface between two fluids
- **Growth rate**: γ ∝ k·Δv (proportional to wavenumber and velocity jump)
- **Standard SPH test**: Tests ability to maintain sharp features and avoid excessive diffusion
- **Benchmark comparisons**: McNally et al. (2012), Hopkins (2015)

## Algorithm Variants

This simulation uses GSPH by default. Can compare with DISPH:
- GSPH: Better for supersonic flows, Riemann solver
- DISPH: Density-independent formulation
- Both should capture instability, but may differ in mixing rate
