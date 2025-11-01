# Hydrostatic Equilibrium Test

## Overview

Tests pressure balance in a 2D hydrostatic equilibrium configuration. This simulation verifies that the SPH method can maintain a stable configuration where pressure forces balance density gradients without external forces.

## Physics

- **Dimension**: 2D
- **SPH Method**: DISPH (Density-Independent SPH)
- **EOS**: Ideal gas (γ = 5/3)
- **Initial Conditions**:
  - Dense central region: ρ = 4.0, r < 0.1
  - Ambient medium: ρ = 1.0, r ≥ 0.1
  - Uniform pressure: P = 2.5 everywhere
  - Particles initially at rest

## Configuration

Default parameters (`config.json`):
- Grid: 32×32 particles
- Domain: [-0.5, 0.5]² with periodic boundaries
- Time: t = 0 → 8.0
- Kernel: Wendland C2
- Artificial viscosity: α = 1.0 with Balsara switch

## Expected Results

In hydrostatic equilibrium:
- Pressure forces should balance density gradients
- Particles should remain nearly stationary
- Total energy should be conserved
- Density profile should remain stable

This tests the ability of the SPH method to:
1. Handle sharp density contrasts
2. Maintain pressure balance
3. Avoid spurious particle motion
4. Preserve equilibrium over long timescales

## Building

```bash
cd /path/to/sphcode
./simulations/hydrostatic/build.sh
```

Or rebuild from project root:
```bash
cd build && cmake --build . --target hydrostatic_plugin
```

## Running

```bash
./build/sph2d simulations/hydrostatic/config.json
```

## Output

- Binary files: `simulations/hydrostatic/output/{run_id}/sph_*.sph`
- Metadata: `simulations/hydrostatic/output/{run_id}/metadata.json`
- Source snapshot: `simulations/hydrostatic/output/{run_id}/source/`

## Analysis

Check if equilibrium is maintained:

```python
from analysis.readers import read_simulation
import matplotlib.pyplot as plt

# Load data
data = read_simulation('simulations/hydrostatic/output/{run_id}')

# Plot density evolution
fig, axes = plt.subplots(1, 2, figsize=(12, 5))

# Initial state
axes[0].scatter(data[0]['x'], data[0]['y'], c=data[0]['rho'], s=1)
axes[0].set_title('t = 0')

# Final state
axes[1].scatter(data[-1]['x'], data[-1]['y'], c=data[-1]['rho'], s=1)
axes[1].set_title(f't = {data[-1]["time"]:.1f}')

plt.show()
```

## Physics References

- Hydrostatic equilibrium: ∇P = -ρ∇Φ (with Φ = 0 for no gravity)
- For constant pressure with density contrast, system should remain static
- Standard test for SPH pressure handling and gradient accuracy
