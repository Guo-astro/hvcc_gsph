# Step 1: Disk Relaxation (GDISPH)

Generate a hydrostatic equilibrium disk using a 2D Lane-Emden profile (n=1.5) with GDISPH hydrodynamics.

## 📁 Directory Structure

```
01_relaxation/
├── README.md                  # This file
├── CMakeLists.txt            # Build configuration
├── Makefile                  # Local build targets (if any)
│
├── config/                   # Configuration files
│   ├── production.json       # Full 500s relaxation
│   └── test.json            # Quick 10s test
│
├── src/                      # Source code
│   ├── plugin.cpp           # Main relaxation plugin
│   └── *.hpp                # Helper headers
│
├── scripts/                  # Python scripts
│   ├── generate_lane_emden_table.py   # Generate theoretical profile
│   └── animate_relaxation.py          # Create visualizations
│
├── data/                     # Input data
│   ├── lane_emden_2d_data.csv        # Lane-Emden solution table
│   └── xi_theta.csv                  # Symlink to above
│
├── docs/                     # Documentation
│   ├── IMPLEMENTATION_COMPLETE.md
│   ├── FLEXIBLE_UNIT_SYSTEM_DESIGN.md
│   └── ...
│
├── output/                   # Simulation outputs (gitignored)
│   └── disk_relaxation/
│       └── run_YYYY-MM-DD_HHMMSS_GDISPH_3d/
│
├── results/                  # Post-processed results
│   ├── animations/          # MP4 animations
│   ├── plots/              # PNG plots
│   └── analysis/           # Analysis outputs
│
└── build/                    # Build artifacts (gitignored)
    └── libdisk_relaxation_plugin.dylib
```

## 🚀 Quick Start

### Build and run test (10s simulation):
```bash
cd /path/to/sphcode/simulations/workflows/razor_thin_hvcc_gdisph_workflow
make step1-test
```

### Run full production relaxation (500s):
```bash
make step1
```

### Generate just the animation from existing output:
```bash
make animate
```

## 📊 Output

- **Simulation data**: `output/disk_relaxation/run_*/`
  - `outputs/csv/*.csv` - Particle snapshots
  - `outputs/binary/*.sph` - Binary snapshots
  - `metadata.json` - Run metadata
  
- **Visualizations**: `results/`
  - `plots/relaxation_comparison.png` - Initial vs final state
  - `animations/relaxation_animation.mp4` - Evolution movie

- **Initial conditions**: `../initial_conditions/relaxed_disk.csv`
  - Final relaxed state used as IC for Step 2 (flyby)

## ⚙️ Configuration

### Test Configuration (`config/test.json`):
- `endTime`: 10.0 (quick test)
- `outputInterval`: 2.0
- `SPHType`: "gdisph"
- `neighborNumber`: 64

### Production Configuration (`config/production.json`):
- `endTime`: 500.0 (full relaxation)
- `outputInterval`: 25.0
- All other parameters same as test

## 🔬 Physics

- **Algorithm**: GDISPH (Godunov SPH)
- **Gravity**: Self-gravity enabled (G = 0.0043)
- **2.5D mode**: Enabled (thin disk approximation)
- **Density relaxation**: Lane-Emden forces to reach equilibrium
- **Profile**: Polytropic disk with n=1.5, γ=5/3
- **Units**: Dimensionless (R=1, M=1, G=1) internally
  - Input/Output: Galactic units (pc, M☉, Myr)

## 📝 Notes

- Flexible unit system automatically handles conversions
- SPHParticle constructor prevents uninitialized smoothing lengths
- Lane-Emden table must be in `data/` directory
- Results are saved to `results/` for version control
