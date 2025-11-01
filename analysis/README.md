# GSPH Analysis Toolkit

Comprehensive Python toolkit for analyzing and visualizing GSPH simulation results.

## Features

### 📊 Conservation Analysis
- **Mass conservation** - Track total mass and relative errors
- **Momentum conservation** - Linear momentum magnitude and direction
- **Angular momentum conservation** - 2D and 3D rotational dynamics
- **Energy conservation** - Kinetic, thermal, and potential energy

### 📐 Theoretical Comparisons
- **Shock tube (Riemann problem)** - Sod shock tube analytical solution
- **Sedov-Taylor blast wave** - Self-similar explosion solution
- **Lane-Emden spheres** - Polytropic equilibrium structures
- Custom theoretical solutions

### 📈 Visualization Tools
- **1D plots** - Line plots with theoretical overlays
- **2D scatter plots** - Colored particle visualization
- **2D interpolated grids** - Smooth field representation
- **3D slices** - 2D cuts through 3D simulations
- **Animations** - MP4 movies of time evolution
- **Conservation plots** - Error evolution over time

## Installation

### Modern Approach with uv (Recommended)

**uv** is a fast Python package manager that replaces pip. It provides better dependency management, faster installation, and reproducible environments.

#### 1. Install uv (if not already installed)
```bash
# macOS/Linux
curl -LsSf https://astral.sh/uv/install.sh | sh

# Or with Homebrew
brew install uv

# Windows
powershell -c "irm https://astral.sh/uv/install.ps1 | iex"
```

#### 2. Set Up Development Environment
```bash
cd /path/to/sphcode

# Create virtual environment and install all dependencies
uv sync

# This creates:
# - .venv/ directory with Python 3.12+
# - uv.lock with exact dependency versions (822KB lockfile)
# - Installs 152 packages including numpy, pandas, scipy, matplotlib
```

#### 3. Verify Installation
```bash
# Test Python imports
uv run python -c "import analysis; import numpy; import matplotlib; print('✅ All imports successful')"

# Test CLI tools
uv run python -m analysis.cli.analyze --help
uv run python -m analysis.cli.animate --help
```

### Traditional Approach with pip

```bash
# Install Python dependencies
pip install numpy pandas scipy matplotlib

# For animations (optional)
brew install ffmpeg  # macOS
apt-get install ffmpeg  # Linux
```

## Quick Start with uv

### Running Analysis Tools

All analysis tools can be run with `uv run`:

```bash
# Quick conservation analysis
uv run python -m analysis.cli.analyze quick results/shock_tube

# Full conservation analysis
uv run python -m analysis.cli.analyze conservation results/shock_tube

# Energy-only analysis
uv run python -m analysis.cli.analyze energy results/shock_tube

# Shock tube comparison
uv run python -m analysis.cli.analyze shock-tube results/shock_tube --gamma 1.4

# Create animation
uv run python -m analysis.cli.animate results/shock_tube -q dens -o shock.mp4
```

### Legacy Script Usage (also works with uv)

```bash
# Quick analysis
uv run python analysis/quick_analysis.py results/shock_tube

# Shock tube analysis
uv run python analysis/shock_tube_analysis.py results/shock_tube 1.4

# Make animation
uv run python analysis/make_animation.py results/shock_tube -q dens --fps 10
```

### Working with the Virtual Environment

```bash
# Activate virtual environment (if you prefer)
source .venv/bin/activate
python -m analysis.cli.analyze quick results/shock_tube

# Or always use uv run (recommended - no activation needed)
uv run python -m analysis.cli.analyze quick results/shock_tube

# Deactivate when done (if activated)
deactivate
```

### Managing Dependencies

```bash
# Add new package
uv add package-name

# Add development dependency
uv add --dev pytest

# Update all packages
uv lock --upgrade

# Sync environment after pulling changes
uv sync
```

## Traditional Quick Start (without uv)

### 1. Run a Simulation
```bash
cd build
./sph shock_tube ../sample/shock_tube/shock_tube.json 8
```

This creates output in `results/shock_tube/`:
- `00000.csv, 00001.csv, ...` - Particle snapshots
- `energy.txt` - Energy history
- `log.txt` - Simulation log

### 2. Quick Analysis
```bash
python analysis/quick_analysis.py results/shock_tube
```

Generates:
- `density_comparison.png` - Initial vs final density
- `energy_conservation.png` - Energy evolution and error
- `conservation_report.png` - Mass, momentum, energy, angular momentum

### 3. Shock Tube Comparison
```bash
python analysis/shock_tube_analysis.py results/shock_tube 1.4
```

Compares simulation with analytical Sod shock tube solution.

### 4. Create Animation
```bash
python analysis/make_animation.py results/shock_tube -q dens -o shock_tube.mp4 --fps 10
```

## Usage Examples

### CLI Tools (Modern Approach)

The toolkit provides a unified command-line interface with subcommands:

#### Quick Analysis
```bash
# Analyze conservation properties and create plots
uv run python -m analysis.cli.analyze quick results/shock_tube

# Outputs:
# - Console: Conservation summary
# - density_comparison.png
# - energy_conservation.png (if energy.txt exists)
# - conservation_report.png
```

#### Conservation Analysis
```bash
# Detailed conservation analysis across all snapshots
uv run python -m analysis.cli.analyze conservation results/shock_tube

# Outputs:
# - Console: Time evolution of conservation errors
# - conservation_full.png: 4-panel plot (mass, momentum, energy, ang. momentum)
```

#### Energy Analysis
```bash
# Energy-only analysis from energy.txt
uv run python -m analysis.cli.analyze energy results/shock_tube

# Outputs:
# - Console: Energy statistics
# - energy_detailed.png: Energy components and error
```

#### Shock Tube Comparison
```bash
# Compare with analytical Sod shock tube solution
uv run python -m analysis.cli.analyze shock-tube results/shock_tube --gamma 1.4

# Outputs:
# - Console: L2 errors at each time
# - shock_tube_comparison.png: Multi-time comparison
# - shock_tube_error.png: Error evolution
```

#### Animation
```bash
# Create MP4 animation
uv run python -m analysis.cli.animate results/shock_tube -q dens -o shock.mp4 --fps 10

# Options:
# -q, --quantity: dens, pres, vel, ene
# -o, --output: Output filename
# --fps: Frames per second
# --interval: Use every Nth snapshot
# --mode: scatter or grid (2D only)
```

### Python Script Usage

#### Load Simulation Data
```python
from analysis import SimulationReader

# Read simulation
reader = SimulationReader('results/shock_tube')
print(f"Dimension: {reader.dim}D")
print(f"Snapshots: {reader.num_snapshots}")

# Read all snapshots
snapshots = reader.read_all_snapshots()
snap = snapshots[0]  # First snapshot

# Access data
print(f"Time: {snap.time}")
print(f"Particles: {snap.num_particles}")
print(f"Positions shape: {snap.pos.shape}")  # (N, DIM)
print(f"Total mass: {snap.total_mass()}")
print(f"Total momentum: {snap.total_momentum()}")
print(f"Kinetic energy: {snap.total_kinetic_energy()}")

# Read energy history
energy = reader.read_energy_history()
print(f"Energy times: {len(energy.time)}")
error = energy.relative_error()
print(f"Max energy error: {max(abs(error)):.6e}")
```

#### Conservation Analysis
```python
from analysis import ConservationAnalyzer

# Analyze conservation
conservation = ConservationAnalyzer.analyze_snapshots(snapshots)

# Print summary
conservation.print_summary()

# Access data
print(f"Mass error: {conservation.mass_error}")
print(f"Momentum error: {conservation.momentum_error}")
print(f"Energy error: {conservation.energy_error}")

# Check energy from file
energy = reader.read_energy_history()
is_conserved = ConservationAnalyzer.check_energy_from_file(
    energy, 
    tolerance=1e-3,
    verbose=True
)
```

#### Theoretical Comparison
```python
from analysis import TheoreticalComparison

# Shock tube comparison
snap = snapshots[-1]  # Final snapshot
solution, error = TheoreticalComparison.compare_shock_tube(
    snap, 
    gamma=1.4, 
    x0=0.0
)
print(f"L2 density error: {error:.6e}")

# Sedov-Taylor comparison (2D/3D)
r_theory, rho_theory, (r_sim, rho_sim), error = \
    TheoreticalComparison.compare_sedov(
        snap,
        E0=1.0,      # Explosion energy
        rho0=1.0,    # Background density
        gamma=1.4
    )
print(f"Sedov L2 error: {error:.6e}")
```

#### Plotting
```python
from analysis import ParticlePlotter, EnergyPlotter
import matplotlib.pyplot as plt

plotter = ParticlePlotter()

# 1D plot
fig, ax = plt.subplots()
plotter.plot_1d(snap, quantity='dens', theory=solution, ax=ax)
plt.savefig('density_1d.png')

# 2D scatter plot
fig, ax = plt.subplots()
plotter.plot_2d_scatter(snap, quantity='dens', cmap='viridis')
plt.savefig('density_2d.png')

# 2D interpolated grid
fig, ax = plt.subplots()
plotter.plot_2d_grid(snap, quantity='pres', grid_size=100)
plt.savefig('pressure_2d.png')

# 3D slice
fig, ax = plt.subplots()
plotter.plot_3d_slice(
    snap, 
    quantity='dens',
    slice_axis=2,          # z-axis
    slice_position=0.0,    # at z=0
    thickness=0.1
)
plt.savefig('density_slice.png')

# Energy plots
energy = reader.read_energy_history()
fig, axes = plt.subplots(2, 1, figsize=(10, 8))
EnergyPlotter.plot_energy_history(energy, ax=axes[0], show_components=True)
EnergyPlotter.plot_energy_error(energy, ax=axes[1])
plt.savefig('energy.png')

# Conservation report
fig = EnergyPlotter.plot_conservation_report(conservation)
plt.savefig('conservation.png')
```

#### Animation
```python
from analysis import AnimationMaker

maker = AnimationMaker(reader)

# 1D animation
anim = maker.animate_1d(
    quantity='dens',
    output_file='shock_1d.mp4',
    fps=10,
    interval=1  # Use every snapshot
)

# 2D animation (scatter)
anim = maker.animate_2d(
    quantity='dens',
    output_file='khi_2d.mp4',
    fps=10,
    mode='scatter',  # or 'grid'
    cmap='viridis'
)
```

## Command-Line Tools

### quick_analysis.py
General-purpose analysis script.

```bash
python analysis/quick_analysis.py <output_dir>
```

**Outputs:**
- Console summary of conservation properties
- `density_comparison.png` - Initial and final density
- `energy_conservation.png` - Energy evolution (if energy.txt exists)
- `conservation_report.png` - Full conservation analysis

**Example:**
```bash
python analysis/quick_analysis.py results/evrard
```

### shock_tube_analysis.py
Shock tube-specific analysis with theoretical comparison.

```bash
python analysis/shock_tube_analysis.py <output_dir> [gamma]
```

**Arguments:**
- `output_dir`: Path to simulation results
- `gamma`: Adiabatic index (default: 1.4)

**Outputs:**
- Console output with L2 errors at each time
- `shock_tube_comparison.png` - Multi-time comparison with theory
- `shock_tube_error.png` - Error evolution

**Example:**
```bash
python analysis/shock_tube_analysis.py results/shock_tube 1.4
```

### make_animation.py
Create MP4 animations.

```bash
python analysis/make_animation.py <output_dir> [options]
```

**Options:**
- `-q, --quantity {dens,pres,vel,ene}` - Quantity to visualize (default: dens)
- `-o, --output FILENAME` - Output file (default: animation.mp4)
- `--fps FPS` - Frames per second (default: 10)
- `--interval N` - Use every Nth snapshot (default: 1)
- `--mode {scatter,grid}` - 2D plotting mode (default: scatter)

**Examples:**
```bash
# Density evolution at 10 fps
python analysis/make_animation.py results/khi -q dens -o khi_dens.mp4

# Pressure with grid interpolation, every 2nd snapshot
python analysis/make_animation.py results/sedov -q pres --mode grid --interval 2

# High framerate for smooth animation
python analysis/make_animation.py results/evrard -q dens --fps 30
```

## Data Structures

### ParticleSnapshot
Single time snapshot of particle data.

**Attributes:**
- `time`: Simulation time
- `num_particles`: Number of particles
- `pos`: Positions, shape (N, DIM)
- `vel`: Velocities, shape (N, DIM)
- `acc`: Accelerations, shape (N, DIM)
- `mass`: Masses, shape (N,)
- `dens`: Densities, shape (N,)
- `pres`: Pressures, shape (N,)
- `ene`: Internal energies, shape (N,)
- `sml`: Smoothing lengths, shape (N,)
- `particle_id`: Particle IDs, shape (N,)

**Optional:**
- `neighbor_count`: Number of neighbors per particle
- `alpha`: Artificial viscosity coefficient
- `shock_sensor`: Shock detection values
- `extra_scalars`: Dict of additional scalar fields
- `extra_vectors`: Dict of additional vector fields

**Methods:**
- `total_mass()`: Sum of particle masses
- `total_momentum()`: Total momentum vector
- `total_kinetic_energy()`: Total kinetic energy
- `total_thermal_energy()`: Total thermal energy
- `center_of_mass()`: Center of mass position
- `center_of_mass_velocity()`: Center of mass velocity

### EnergyHistory
Energy evolution from energy.txt.

**Attributes:**
- `time`: Time array
- `kinetic`: Kinetic energy array
- `thermal`: Thermal energy array
- `potential`: Potential energy array
- `total`: Total energy array

**Methods:**
- `relative_error(reference_time=0.0)`: Energy error relative to reference

### ConservationReport
Conservation analysis results.

**Attributes:**
- `time`: Time array
- `total_mass`, `mass_error`: Mass conservation
- `momentum`, `momentum_error`: Momentum conservation
- `angular_momentum`, `angular_momentum_error`: Angular momentum (2D/3D)
- `kinetic_energy`, `thermal_energy`, `total_energy`, `energy_error`: Energy

**Methods:**
- `summary()`: Dict of max errors
- `print_summary()`: Print formatted summary

## Advanced Usage

### Custom Theoretical Solutions

```python
from analysis import TheoreticalComparison

# Define custom solution
def my_theoretical_solution(x, t):
    """Custom analytical solution."""
    rho = np.exp(-(x - t)**2)
    vel = np.ones_like(x)
    pres = rho**1.4
    ene = pres / (0.4 * rho)
    return rho, vel, pres, ene

# Compare with simulation
snap = snapshots[-1]
x = snap.pos[:, 0]
rho_theory, vel_theory, pres_theory, ene_theory = my_theoretical_solution(x, snap.time)

# Compute error
error = np.sqrt(np.mean((snap.dens - rho_theory)**2))
print(f"L2 error: {error:.6e}")

# Plot
import matplotlib.pyplot as plt
fig, ax = plt.subplots()
ax.scatter(x, snap.dens, label='Simulation')
ax.plot(x, rho_theory, 'r-', label='Theory')
ax.legend()
plt.show()
```

### Batch Analysis

```python
import glob
from pathlib import Path

# Analyze multiple simulations
result_dirs = glob.glob('results/*/')

for result_dir in result_dirs:
    print(f"\nAnalyzing {result_dir}...")
    
    reader = SimulationReader(result_dir)
    snapshots = reader.read_all_snapshots()
    
    conservation = ConservationAnalyzer.analyze_snapshots(snapshots)
    summary = conservation.summary()
    
    # Save summary to file
    with open(Path(result_dir) / 'conservation_summary.txt', 'w') as f:
        for key, value in summary.items():
            f.write(f"{key}: {value:.6e}\n")
```

### Extract Specific Particles

```python
# Track specific particles over time
particle_ids = [0, 10, 100]

positions = {pid: [] for pid in particle_ids}
velocities = {pid: [] for pid in particle_ids}
times = []

for snap in snapshots:
    times.append(snap.time)
    for pid in particle_ids:
        idx = np.where(snap.particle_id == pid)[0][0]
        positions[pid].append(snap.pos[idx])
        velocities[pid].append(snap.vel[idx])

# Plot trajectories
import matplotlib.pyplot as plt
fig, ax = plt.subplots()
for pid in particle_ids:
    traj = np.array(positions[pid])
    ax.plot(traj[:, 0], traj[:, 1], label=f'Particle {pid}')
ax.legend()
ax.set_xlabel('x')
ax.set_ylabel('y')
ax.set_title('Particle Trajectories')
plt.show()
```

## Troubleshooting

### ModuleNotFoundError
```bash
# Make sure you're running from sphcode directory
cd /path/to/sphcode
python analysis/quick_analysis.py results/shock_tube
```

### Animation fails with "ffmpeg not found"
```bash
# Install ffmpeg
brew install ffmpeg  # macOS
sudo apt-get install ffmpeg  # Linux
```

### Memory issues with large simulations
```python
# Read subset of snapshots
reader = SimulationReader('results/large_sim')
indices = range(0, reader.num_snapshots, 10)  # Every 10th snapshot
snapshots = [reader.read_snapshot(i) for i in indices]
```

### Custom output directory
Toolkit assumes output in `results/<sim_name>/` but works with any directory:
```bash
python analysis/quick_analysis.py /custom/path/to/output
```

## Tips and Best Practices

1. **Check energy first**: Always verify energy conservation with `energy.txt`
2. **Use appropriate tolerance**: `1e-3` is typical, but adjust based on problem
3. **Interpolate carefully**: Grid interpolation smooths data - use scatter for accuracy
4. **Animate wisely**: Use `--interval` to skip snapshots for faster animation
5. **Compare with theory**: Always validate against known solutions when available

## Integration with GSPH

The analysis toolkit reads standard GSPH output:
- Works automatically with all simulations
- No code changes needed in C++ side
- Extensible for custom output fields

To add custom fields to analysis:
1. Add field to `sim->add_scalar_array()` or `sim->add_vector_array()` in C++
2. Toolkit automatically reads and makes available in `extra_scalars`/`extra_vectors`

## Examples by Problem Type

### Shock Tube (1D)
```bash
./sph shock_tube sample/shock_tube/shock_tube.json 8
python analysis/shock_tube_analysis.py results/shock_tube 1.4
python analysis/make_animation.py results/shock_tube -q dens
```

### Kelvin-Helmholtz Instability (2D)
```bash
./sph khi sample/khi/khi.json 8
python analysis/quick_analysis.py results/khi
python analysis/make_animation.py results/khi -q dens --mode grid --fps 15
```

### Evrard Collapse (3D)
```bash
./sph evrard sample/evrard/evrard.json 8
python analysis/quick_analysis.py results/evrard
# 3D: plot slices manually
```

### Sedov-Taylor Blast (2D/3D)
```bash
./sph sedov_taylor sample/sedov_taylor/sedov_taylor.json 8
python analysis/quick_analysis.py results/sedov_taylor
# Custom script needed for Sedov comparison
```

## Future Enhancements

Planned features:
- [ ] Jupyter notebook examples
- [ ] 3D volume rendering
- [ ] Spectral analysis tools
- [ ] Comparison with observations
- [ ] Power spectrum calculation
- [ ] Structure identification (halos, shocks)

## Contributing

To add new features:
1. Add method to appropriate module (readers.py, conservation.py, etc.)
2. Add example usage to this README
3. Create test script in `analysis/examples/`

---

For questions or issues, see DEVELOPER_GUIDE.md or open an issue.
