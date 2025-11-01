# Production Simulations

Research-grade simulations for scientific publications and data production. These are typically more complex, higher resolution, and longer duration than test cases.

## Purpose

Production simulations are designed for:
- **Scientific research**: Generate publishable results
- **Parameter studies**: Systematic exploration of parameter space
- **High resolution**: Large particle counts (10^5 - 10^7+)
- **Long duration**: Many dynamical timescales
- **Complex physics**: Multiple modules enabled (gravity, cooling, etc.)

## Subdirectories

### Razor Thin HVCC (`razor_thin_hvcc/`)

High-velocity cloud collision in a razor-thin disk geometry.

**Simulations:**
- `razor_thin_hvcc.cpp` - Main production run
- `razor_thin_hvcc_debug.cpp` - Debug version with extra diagnostics

**Physical Setup:**
- Thin galactic disk (H/R ~ 0.03)
- High-velocity cloud (HVC) impact
- Self-gravity enabled
- Potentially includes cooling, magnetic fields

**Science Goals:**
- Cloud fragmentation and star formation
- Energy dissipation in collision
- Disk response to perturbation
- Gravitational instability triggering

**Typical Parameters:**
- N_particles: 10^5 - 10^6
- Simulation time: 10-100 Myr
- Include: gravity, cooling, potentially feedback

### Razor Thin Relaxation (`razor_thin_relaxation/`)

Relaxation of razor-thin disk to stable equilibrium.

**Simulations:**
- `razor_thin_sg_relaxation.cpp` - Self-gravitating disk relaxation

**Purpose:**
- Generate equilibrium initial conditions
- Remove transients from idealized setup
- Establish stable disk before perturbations

**Process:**
1. Start with analytical disk profile
2. Let system relax (10-50 orbits)
3. Check for long-term stability
4. Use relaxed state as IC for science runs

### Blast Waves (`blast_waves/`)

Supernova-like blast waves in disks or clouds.

**Simulations:**
- `test_razor_thin_blast_wave.cpp` - Blast wave in thin disk

**Physical Setup:**
- Energy injection in small region
- Shock wave expansion
- Interaction with ambient medium

**Science Goals:**
- Supernova feedback in disks
- Bubble expansion
- Momentum injection
- Energy dissipation

## Production Workflow

### 1. Setup Phase
```bash
# Create initial conditions
./sph razor_thin_sg_relaxation configs/production/relax.json 8

# Check relaxation quality
uv run gsph-analyze conservation results/relax
```

### 2. Production Run
```bash
# Run on HPC cluster with many cores
./sph razor_thin_hvcc configs/production/hvcc_run1.json 64

# Monitor progress
tail -f results/hvcc_run1/energy.txt
```

### 3. Analysis Phase
```bash
# Full conservation analysis
uv run gsph-analyze conservation results/hvcc_run1

# Create visualizations
uv run gsph-animate results/hvcc_run1 -q dens,temp,vel

# Custom analysis scripts
python analysis/custom_hvcc_analysis.py results/hvcc_run1
```

## Best Practices

### Resolution

**Minimum Requirements:**
- **Jeans Length**: λ_J > 4h (resolve gravitational fragmentation)
- **Shocks**: 10+ particles across shock front
- **Disk Scale Height**: H > 10h (vertical structure)
- **Features**: 100+ particles in regions of interest

**Particle Counts:**
- Quick tests: 10^4 - 10^5
- Standard: 10^5 - 10^6
- High-res: 10^6 - 10^7
- Ultra-high-res: 10^7+

### Timestep Management

**CFL Numbers:**
- Sound speed: 0.2 - 0.3 (conservative)
- Force: 0.1 - 0.15
- Reduce for stability in extreme conditions

**Output Cadence:**
- Frequent enough to capture dynamics
- Not so frequent that I/O dominates
- Adaptive: more frequent during interesting events

**Recommended:**
- Output every 0.01 - 0.1 t_dyn
- Extra snapshots around key events
- Full dumps + light output for animations

### Checkpointing

**Restart Capability:**
- Save full state every N timesteps
- Include particle positions, velocities, energies
- Include random number state (if stochastic processes)
- Metadata: time, step number, git commit hash

**Frequency:**
- Every 1-24 hours of wall time
- Before potential crashes (long runs)
- After expensive relaxation phases

### Data Management

**Storage Requirements:**
- Estimate: N_particles × N_snapshots × bytes_per_particle
- Example: 10^6 particles × 1000 snaps × 100 bytes = 100 GB

**Compression:**
- HDF5 with compression
- Store only necessary fields
- Downsampling for visualization

**Backup:**
- Critical simulations: 3-2-1 rule
- Archive final data products
- Document processing pipeline

## Configuration Guidelines

### Self-Gravity

```json
{
  "useGravity": true,
  "G": 4.498e-3,  // In code units
  "theta": 0.5,    // Barnes-Hut opening angle
  "gravitationalSoftening": 0.01,  // ~2.8h
  "leafParticleNumber": 16
}
```

### Cooling

```json
{
  "useHeatingCooling": true,
  "coolingFunction": "table",
  "coolingTableFile": "cooling_tables/solar_metallicity.dat",
  "temperatureFloor": 10.0,  // Kelvin
  "temperatureCeiling": 1e8
}
```

### High Resolution

```json
{
  "neighborNumber": 64,  // Higher for better accuracy
  "useBalsaraSwitch": true,  // Reduce AV in shear
  "useTimeDependentAV": true,  // Adapt AV dynamically
  "cflSound": 0.25,  // Conservative
  "outputTime": 0.01  // Frequent outputs
}
```

## Performance Optimization

### OpenMP Parallelization

```bash
# Use all cores
export OMP_NUM_THREADS=$(nproc)

# Or specify
export OMP_NUM_THREADS=32

# Run
./sph simulation config.json $OMP_NUM_THREADS
```

### Load Balancing

- Tree builds: ~10-20% of runtime
- Neighbor search: ~20-30%
- Force calculation: ~40-50%
- I/O: minimize to <5%

### Profiling

```bash
# Basic timing
time ./sph simulation config.json 8

# Detailed profiling (if built with -pg)
gprof sph gmon.out > profile.txt

# Or use perf
perf record ./sph simulation config.json 8
perf report
```

## Validation Checklist

Before publishing results, verify:

- [ ] Energy conservation < 1% over full run
- [ ] Momentum conservation < 0.1%
- [ ] Angular momentum conservation (if applicable)
- [ ] Resolution study: results converge with N?
- [ ] Parameter study: results robust to AV, h, etc.?
- [ ] Comparison to previous work (if applicable)
- [ ] Physical sanity checks (timescales, energies)

## Documentation Requirements

Each production simulation should have:

1. **README.md** in simulation directory
   - Physics description
   - Parameter choices and justification
   - Expected outcomes
   - Known issues

2. **Configuration Files**
   - Well-commented JSON
   - Version controlled
   - Include units clearly

3. **Analysis Scripts**
   - Reproducible analysis pipeline
   - Documented in Python/Jupyter
   - Version controlled

4. **Results Summary**
   - Key findings
   - Figures and movies
   - Data availability

## Common Issues

### Issue: Simulation crashes after days
- **Prevention**: Regular checkpointing
- **Recovery**: Restart from last checkpoint
- **Debug**: Enable extra logging, run smaller test case

### Issue: Results don't match expectations
- **Check**: Initial conditions (relaxed?)
- **Check**: Parameter choices (reasonable?)
- **Check**: Code version (correct branch/commit?)
- **Compare**: To simpler test cases

### Issue: Running too slowly
- **Profile**: Find bottleneck
- **Optimize**: Neighbor search parameters
- **Consider**: Reducing resolution temporarily
- **Hardware**: Move to HPC cluster

## Contact and Support

For production simulation support:
- **Code issues**: Open GitHub issue
- **Physics questions**: Consult collaborators
- **HPC problems**: Contact cluster support
- **Data analysis**: See analysis/README.md

## Citation

If you use these simulations for publication, please cite:
- **Code**: [GSPHCODE citation]
- **Methods**: Hopkins (2015) for GSPH, etc.
- **This work**: [Your paper]
