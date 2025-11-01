# HVCC Workflow Output Guide

## 📁 Directory Structure

```
/Users/guo/OSS/sphcode/simulations/workflows/razor_thin_hvcc_workflow/
│
├── 📊 visualizations/          ← ALL VISUALIZATION OUTPUT HERE
│   ├── run_*_DISPH_3d/        ← Per-run visualizations
│   │   ├── snapshot_*.png      ← Individual detailed plots
│   │   ├── evolution_comparison.png
│   │   └── frames/             ← Movie frames (PNG)
│   └── initial_conditions_overview.png
│
├── 📁 01_relaxation/
│   ├── output/disk_relaxation/run_*/  ← Simulation raw data
│   │   ├── outputs/
│   │   │   ├── csv/            ← CSV snapshots (READABLE)
│   │   │   └── binary/         ← Binary snapshots (COMPRESSED)
│   │   ├── config.json         ← Run configuration
│   │   ├── initial_conditions.csv
│   │   └── metadata.json
│   ├── lane_emden_2d_data.csv  ← Physics table
│   └── xi_theta.csv            ← Symlink
│
├── 📁 initial_conditions/       ← Shared between workflow steps
│   ├── relaxed_disk_test.csv   ← Ready for Step 2
│   └── README.md
│
├── 📁 02_flyby/                 ← Step 2 (not yet run)
│
├── 🎬 create_visualizations.py  ← Regenerate all visualizations
├── 🎨 visualize_disk.py         ← Single-file visualization
└── 📖 README.md
```

## 📊 Visualization Files

### Location
**All visualizations**: `/Users/guo/OSS/sphcode/simulations/workflows/razor_thin_hvcc_workflow/visualizations/`

### Generated Files

1. **Individual Snapshot Plots** (`snapshot_NNNNN.png`)
   - 6-panel detailed view
   - Face-on (XY), Edge-on (XZ), Radial profile
   - Pressure, Velocity, Statistics

2. **Evolution Comparison** (`evolution_comparison.png`)
   - Side-by-side comparison of all snapshots
   - Consistent color scaling
   - Shows face-on and edge-on views

3. **Movie Frames** (`frames/frame_*.png`)
   - Individual frames for animation
   - Consistent axes and colors
   - Ready for ffmpeg processing

4. **Movies** (`run_*_relaxation.mp4`)
   - Automated if ffmpeg works
   - Note: Current runs failed due to odd dimensions
   - Can be manually created (see below)

## 📈 Raw Data Files

### CSV Snapshots
**Location**: `01_relaxation/output/disk_relaxation/run_*/outputs/csv/`

**Format**: Human-readable CSV with headers
```csv
# Units: distance[pc], velocity[km/s], mass[M_sun], density[M_sun/pc^3], ...
x,y,z,vx,vy,vz,rho,p,u,h,mass,ax,ay,az
0.120000,0.000000,0.000000,0.000000,0.000000,0.000000,45.789,...
```

**Columns**:
- Position: `x, y, z` (pc)
- Velocity: `vx, vy, vz` (km/s)  
- Hydro: `rho, p, u` (density, pressure, internal energy)
- SPH: `h, mass` (smoothing length, particle mass)
- Acceleration: `ax, ay, az`

### Binary Snapshots
**Location**: `01_relaxation/output/disk_relaxation/run_*/outputs/binary/`

**Format**: Custom binary format (`.sph` files)
- ~0.7x compression vs CSV
- Use `analysis/binary_reader.py` to read
- Faster I/O for large simulations

## 🎬 Creating Movies Manually

If `create_visualizations.py` fails to create movies:

```bash
cd visualizations/run_2025-11-01_153056_DISPH_3d/frames

# Method 1: Using ffmpeg with scaling (fixes dimension issue)
ffmpeg -framerate 2 -pattern_type glob -i 'frame_*.png' \
  -vf "scale=1184:494" \  # Make dimensions even
  -c:v libx264 -pix_fmt yuv420p -crf 23 \
  ../disk_relaxation.mp4

# Method 2: Using GIF (no dimension requirements)
ffmpeg -framerate 2 -pattern_type glob -i 'frame_*.png' \
  -vf "scale=800:-1" \  # Resize for smaller file
  ../disk_relaxation.gif
```

## 🔄 Regenerating Visualizations

```bash
cd /Users/guo/OSS/sphcode/simulations/workflows/razor_thin_hvcc_workflow

# Regenerate everything
python create_visualizations.py

# Visualize a specific snapshot
python visualize_disk.py initial_conditions/relaxed_disk_test.csv

# Visualize from specific run
python visualize_disk.py 01_relaxation/output/disk_relaxation/run_*/outputs/csv/00004.csv
```

## 📊 Latest Successful Run

**Run**: `run_2025-11-01_153056_DISPH_3d`
**Snapshots**: 5 (00000 to 00004)
**Particles**: 9,880
**Physics**: 2.5D disk relaxation with Lane-Emden profile

**Visualizations**:
- ✅ 5 individual snapshot PNGs
- ✅ Evolution comparison PNG
- ✅ 5 movie frames
- ⚠️ MP4 creation failed (odd dimensions - can be fixed)

## 🔍 Quick Analysis

```python
import numpy as np
import matplotlib.pyplot as plt

# Read CSV
data = np.loadtxt('initial_conditions/relaxed_disk_test.csv', 
                  delimiter=',', skiprows=1)

# Extract columns
x, y, z = data[:, 0], data[:, 1], data[:, 2]
rho, p, u = data[:, 6], data[:, 7], data[:, 8]
mass = data[:, 10]

# Plot density
plt.figure(figsize=(10, 5))
plt.subplot(121)
plt.scatter(x, y, c=rho, s=1, cmap='viridis')
plt.colorbar(label='Density')
plt.xlabel('x (pc)')
plt.ylabel('y (pc)')
plt.title('Face-on')
plt.axis('equal')

plt.subplot(122)
plt.scatter(x, z, c=rho, s=1, cmap='viridis')
plt.colorbar(label='Density')
plt.xlabel('x (pc)')
plt.ylabel('z (pc)')
plt.title('Edge-on')
plt.axis('equal')

plt.tight_layout()
plt.show()
```

## 📝 Notes

1. **Timestep Issue**: The relaxation simulation shows a timestep explosion at snapshot 4, causing the simulation to end prematurely. This is visible in the evolution plot where the disk collapses dramatically.

2. **File Locations**: All outputs are consolidated in the workflow root directory for easy access.

3. **Analysis Tools**: Use the scripts in `/Users/guo/OSS/sphcode/analysis/` for more advanced analysis.

4. **Next Steps**: 
   - Fix timestep limiting in the relaxation phase
   - Run Step 2 (flyby) with the relaxed IC
   - Compare DISPH vs GDISPH methods
