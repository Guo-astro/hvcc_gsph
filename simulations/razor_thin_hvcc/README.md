# Razor-Thin HVCC Flyby Simulation

## Overview

Production-quality simulation of a high-velocity cloud complex (HVCC) encountering an intermediate-mass black hole (IMBH). This simulation demonstrates:
- Checkpoint-based initialization (loads relaxed disk)
- Point mass particles (IMBH treated as collisionless particle)
- Self-gravity with 2.5D approximation
- Flyby dynamics and tidal disruption

## Physics

- **Dimension**: 3D (with 2.5D mode for thin disk)
- **SPH Method**: DISPH (Density-Independent SPH)
- **Gravity**: Self-gravity + point mass (IMBH)
- **EOS**: Ideal gas (γ = 5/3)
- **Initial Conditions**:
  - Relaxed disk loaded from checkpoint file
  - Static IMBH added at impact parameter
  - Disk given infall velocity toward IMBH
  - High resolution (~1M particles)

## Configuration

Default parameters (`config.json`):
- Resolution: ~1,000,000 particles (loaded from checkpoint)
- Time: t = 0 → 30.0
- Kernel: Wendland C2
- Neighbor number: 2048 (high for gravity calculations)
- Artificial viscosity: α = 2.0 with Balsara switch
- **Checkpoint file**: Must be set to path of relaxed disk

### Flyby Parameters

- `impact_parameter`: Distance of closest approach (parsecs)
- `initial_velocity`: Initial infall speed (km/s)
- `point_mass`: IMBH mass (solar masses)

## Expected Results

The simulation captures flyby dynamics:
1. **Approach phase**: Disk falls toward IMBH
2. **Periapsis**: Tidal forces strongest at closest approach
3. **Disruption**: Disk material stripped and disturbed
4. **Post-flyby**: Remnant structure and potential bound material

Key physics:
- Tidal disruption radius: r_t ~ (M_disk/M_IMBH)^(1/3) × R_disk
- Orbital dynamics and angular momentum transfer
- Shock heating from compression
- Potential accretion onto IMBH

## Building

```bash
cd /path/to/sphcode
./simulations/razor_thin_hvcc/build.sh
```

Or rebuild from project root:
```bash
cd build && cmake --build . --target razor_thin_hvcc_plugin
```

**Note**: This plugin requires DIM=3 to be built.

## Running

**IMPORTANT**: This simulation requires a checkpoint file with relaxed disk initial conditions.

1. Set the initial conditions file path in `config.json`:
```json
{
    "initialConditionsFile": "path/to/relaxed_disk.csv",
    ...
}
```

2. Run the simulation:
```bash
./build/sph3d simulations/razor_thin_hvcc/config.json
```

## Initial Conditions File Format

The checkpoint file should be a CSV with columns:
- `id`: Particle ID
- `x, y, z`: Position
- `vx, vy, vz`: Velocity
- `rho`: Density
- `P`: Pressure
- `m`: Mass
- Additional columns as needed

The `InfallModifier` will:
1. Load these particles
2. Add an IMBH particle at the target position
3. Modify velocities for infall dynamics
4. Offset positions for initial separation

## Output

- Binary files: `simulations/razor_thin_hvcc/output/{run_id}/sph_*.sph`
- Metadata: `simulations/razor_thin_hvcc/output/{run_id}/metadata.json`
- Source snapshot: `simulations/razor_thin_hvcc/output/{run_id}/source/`

## Analysis

Track IMBH and disk evolution:

```python
from analysis.readers import read_simulation
import matplotlib.pyplot as plt
import numpy as np

# Load data
data = read_simulation('simulations/razor_thin_hvcc/output/{run_id}')

# Extract IMBH position (particle with is_point_mass flag)
# Assumes IMBH is last particle or marked specially

# Plot disk density evolution
for i, snapshot in enumerate(data):
    if i % 10 == 0:
        fig = plt.figure(figsize=(12, 4))
        
        # XY view
        ax1 = fig.add_subplot(131)
        ax1.scatter(snapshot['x'], snapshot['y'], c=snapshot['rho'], s=0.1)
        ax1.set_aspect('equal')
        ax1.set_title(f't = {snapshot["time"]:.2f}')
        
        # XZ view
        ax2 = fig.add_subplot(132)
        ax2.scatter(snapshot['x'], snapshot['z'], c=snapshot['rho'], s=0.1)
        ax2.set_aspect('equal')
        
        # YZ view
        ax3 = fig.add_subplot(133)
        ax3.scatter(snapshot['y'], snapshot['z'], c=snapshot['rho'], s=0.1)
        ax3.set_aspect('equal')
        
        plt.tight_layout()
        plt.savefig(f'hvcc_frame_{i:04d}.png')
        plt.close()
```

Measure tidal disruption:
```python
# Compute distance from IMBH for each particle
imbh_pos = data[-1]['pos'][data[-1]['is_point_mass']]
distances = np.sqrt((data[-1]['x'] - imbh_pos[0])**2 + 
                   (data[-1]['y'] - imbh_pos[1])**2 +
                   (data[-1]['z'] - imbh_pos[2])**2)

# Identify bound vs. unbound material
# (requires computing specific energies)
```

## Implementation Details

### InfallModifier

The `InfallModifier` class extends `InitialConditionsModifier`:
- Adds IMBH particle on first call
- Sets infall velocities for all fluid particles
- Offsets positions for initial separation

### 2.5D Mode

The simulation uses `two_and_half_sim = true`:
- Particles constrained to thin layer in z-direction
- Gravity computed assuming extended disk structure
- Reduces computational cost for thin disk systems

### Particle Types

- **Fluid particles**: SPH particles from initial conditions file
- **Point mass**: IMBH particle marked with `is_point_mass = true`
  - Contributes to gravity but not hydrodynamics
  - Has mass but no pressure, density, or internal energy

## Physics References

- **HVCC Flybys**: High-velocity cloud-IMBH interactions
- **Tidal Disruption**: Roche limit and tidal radius calculations
- **2.5D Gravity**: Thin disk approximation for computational efficiency
- **Astrophysical Applications**: Galactic center dynamics, cloud-BH interactions

## Parameter Guidelines

- **Impact parameter**: 0.5 - 2.0 pc (affects tidal strength)
- **Infall velocity**: 5 - 50 km/s (typical HVCC speeds)
- **IMBH mass**: 10³ - 10⁶ M☉ (intermediate-mass range)
- **Neighbor number**: ≥ 1000 for accurate gravity (trades off with performance)

## Troubleshooting

**Missing checkpoint file**: 
```
Error: Cannot open checkpoint file
```
Solution: Set valid path in `config.json` or create relaxed disk using separate relaxation simulation.

**DIM mismatch**:
```
Error: Razor-thin HVCC requires DIM = 3
```
Solution: Rebuild with `cmake -DDIM=3` or use provided `build.sh` script.

**High memory usage**:
With ~1M particles and 2048 neighbors, expect ~10-20 GB RAM usage.
Reduce `neighborNumber` if needed (but affects gravity accuracy).
