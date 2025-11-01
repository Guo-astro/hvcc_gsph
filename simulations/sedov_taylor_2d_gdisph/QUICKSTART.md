# Quick Start: 2D Sedov-Taylor Blast Wave

## Build the Plugin

```bash
cd simulations/sedov_taylor_2d
chmod +x build.sh
./build.sh
```

This creates: `build/libsedov_taylor_2d_plugin.dylib`

## Run the Simulation

```bash
cd ../../build  # Main build directory
./sph2d ../simulations/sedov_taylor_2d/build/libsedov_taylor_2d_plugin.dylib
```

Output appears in: `simulations/sedov_taylor_2d/run_YYYY-MM-DD_HHMMSS_DISPH_2d/`

## Compare with Analytical Solution

After simulation completes:

```bash
cd ../simulations/sedov_taylor_2d

# Compare latest run with analytical solution
python analytical/sedov_taylor_solution.py --run latest --dimension 2

# Or specify a specific run
python analytical/sedov_taylor_solution.py --run run_2025-11-01_111312_DISPH_2d --dimension 2
```

This generates comparison plots in: `<run_dir>/analysis/sedov_comparison_t*.png`

## What to Expect

**Simulation**:
- 10,000 particles (100×100 grid)
- ~10 output snapshots
- Runtime: ~1-2 minutes
- Circular shock wave expanding from center

**Analytical Solution**:
- Radial profiles: density, velocity, pressure
- 2D spatial distribution maps
- Shock position vs time
- Energy conservation check

## Validation

Check these metrics:

1. **Shock Radius**: Should follow R(t) = ξ₀ t^0.4 (2D scaling)
2. **Density Jump**: ρ_shock/ρ_ambient ≈ 6.0 (for γ=1.4)
3. **Circular Symmetry**: Perfectly circular shock front
4. **Energy Conservation**: E_total ≈ 1.0 throughout

## Next Steps

- Edit `config.json` to change resolution or runtime
- Compare DISPH (2D) with GSPH (3D version)
- Create animations: `python ../../analysis/make_animation.py latest`
- Study shock evolution with different γ values

## Files Created

After running, you'll have:

```
simulations/sedov_taylor_2d/
└── run_2025-11-01_HHMMSS_DISPH_2d/
    ├── metadata.json              # Run information
    ├── config.json                # Configuration used
    ├── initial_conditions.csv     # t=0 state
    ├── source/
    │   └── sedov_taylor_2d.cpp   # Source code snapshot
    ├── outputs/
    │   ├── csv/                   # Human-readable snapshots
    │   │   ├── 00000.csv
    │   │   └── ...
    │   └── binary/                # Compact binary format
    │       ├── 00000.sph
    │       └── ...
    ├── analysis/                  # Comparison plots
    │   └── sedov_comparison_t*.png
    ├── visualizations/            # (if you create animations)
    └── logs/                      # Simulation output
```

## Troubleshooting

**Plugin won't load?**
- Make sure `build.sh` completed successfully
- Check that `build/libsedov_taylor_2d_plugin.dylib` exists
- Verify you're using `sph2d` (not `sph1d` or `sph3d`)

**Analytical comparison fails?**
- Install dependencies: `pip install numpy scipy matplotlib`
- Make sure simulation completed (check for output files)
- Verify the run directory exists

**Simulation crashes?**
- Check system has enough memory (~100 MB for 10k particles)
- Look at logs in the run directory
- Try reducing particle count in `sedov_taylor_2d.cpp`
