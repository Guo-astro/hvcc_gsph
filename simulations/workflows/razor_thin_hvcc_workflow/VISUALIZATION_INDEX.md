# 🎬 Workflow Visualizations & Raw Data

## Quick Access

**Open the interactive viewer:**
```bash
open visualizations/index.html
```

Or directly browse:
```bash
cd /Users/guo/OSS/sphcode/simulations/workflows/razor_thin_hvcc_workflow
```

---

## 📊 All Animations and Visualizations

### Step 1: Disk Relaxation

All visualizations are in: `visualizations/`

#### ✅ Available Now:

1. **run_2025-11-01_145554_DISPH_3d** (1 snapshot)
   - �� Evolution plot: `visualizations/run_*/evolution_comparison.png`
   - 🎬 Animation: `visualizations/run_*/run_*_relaxation.mp4`
   - 📁 Raw CSV: `01_relaxation/output/disk_relaxation/run_*/outputs/csv/`

2. **run_2025-11-01_150056_DISPH_3d** (1 snapshot)
   - Same structure as above

3. **run_2025-11-01_152234_DISPH_3d** (1 snapshot)
   - Same structure as above

4. **run_2025-11-01_153038_DISPH_3d** (1 snapshot)
   - Same structure as above

5. **run_2025-11-01_153056_DISPH_3d** (5 snapshots) ⭐ **LATEST**
   - 📊 Evolution plot: Shows disk collapse over time
   - 🎬 Animation: 5-frame MP4 movie
   - 📸 Individual snapshots: `snapshot_00000.png` through `snapshot_00004.png`
   - 📁 Raw CSV: 5 snapshots (2.4 MB → 1.8 MB each)

#### Initial Conditions:
- 📸 Overview: `visualizations/initial_conditions_overview.png`
- 📁 CSV Data: `initial_conditions/relaxed_disk_test.csv` (9,880 particles, 1.8 MB)

### Step 2: Flyby Encounter

⚠️ **Not yet run** - Ready to execute with initial conditions from Step 1

---

## 📁 Raw Data Locations

### CSV Snapshots (Human-Readable)
```
01_relaxation/output/disk_relaxation/
├── run_2025-11-01_145554_DISPH_3d/outputs/csv/00000.csv
├── run_2025-11-01_150056_DISPH_3d/outputs/csv/00000.csv
├── run_2025-11-01_152234_DISPH_3d/outputs/csv/00000.csv
├── run_2025-11-01_153038_DISPH_3d/outputs/csv/00000.csv
└── run_2025-11-01_153056_DISPH_3d/outputs/csv/
    ├── 00000.csv  (2.4 MB)
    ├── 00001.csv  (2.2 MB)
    ├── 00002.csv  (1.9 MB)
    ├── 00003.csv  (1.8 MB)
    └── 00004.csv  (1.8 MB)

Total: 9 CSV files, 19.7 MB
```

### Binary Snapshots (Compressed)
```
01_relaxation/output/disk_relaxation/
└── run_*/outputs/binary/*.sph

Total: 9 binary files, 12.6 MB
```

### CSV Format
```csv
# Units: distance[pc], velocity[km/s], mass[M_sun], density[M_sun/pc^3]
x,y,z,vx,vy,vz,rho,p,u,h,mass,ax,ay,az
0.120000,0.000000,0.000000,0.000000,0.000000,0.000000,45.789,...
```

**Columns:**
- Position: `x, y, z` (pc)
- Velocity: `vx, vy, vz` (km/s)
- Hydrodynamics: `rho` (density), `p` (pressure), `u` (internal energy)
- SPH: `h` (smoothing length), `mass` (particle mass)
- Acceleration: `ax, ay, az`

---

## 🎨 Visualization Types

### 1. Evolution Comparison Plots
Side-by-side comparison of all snapshots in a run
- Face-on view (XY plane)
- Edge-on view (XZ plane)
- Consistent color scaling for easy comparison

### 2. Individual Snapshot Plots
Detailed 6-panel view for each snapshot:
- Face-on density
- Edge-on density
- Radial density profile
- Pressure distribution
- Velocity field
- Statistics panel

### 3. MP4 Movies
Smooth animations showing disk evolution
- 2 fps for detailed viewing
- H.264 codec
- 1200x600 pixels (even dimensions for compatibility)

### 4. Movie Frames
Individual PNG frames for each snapshot
- Consistent axes ranges
- Consistent colormaps
- Located in `visualizations/run_*/frames/`

---

## 🔄 Regenerate Visualizations

```bash
cd /Users/guo/OSS/sphcode/simulations/workflows/razor_thin_hvcc_workflow

# Regenerate all visualizations
python create_visualizations.py

# View comprehensive report
python workflow_viewer.py

# Visualize a specific file
python visualize_disk.py initial_conditions/relaxed_disk_test.csv
```

---

## 📈 Statistics Summary

| Metric | Value |
|--------|-------|
| **Total Runs** | 5 |
| **Total Snapshots** | 9 |
| **Particles** | 9,880 |
| **PNG Plots** | 24 |
| **MP4 Movies** | 5 |
| **Total Raw Data** | 32.3 MB |
| **CSV Data** | 19.7 MB |
| **Binary Data** | 12.6 MB |
| **Visualizations Size** | 2.2 MB |

---

## 🎯 Quick Commands

### View Latest Animation
```bash
open visualizations/run_2025-11-01_153056_DISPH_3d/run_2025-11-01_153056_DISPH_3d_relaxation.mp4
```

### View Latest Evolution Plot
```bash
open visualizations/run_2025-11-01_153056_DISPH_3d/evolution_comparison.png
```

### Browse All Snapshots
```bash
open visualizations/run_2025-11-01_153056_DISPH_3d/snapshot_00000.png
open visualizations/run_2025-11-01_153056_DISPH_3d/snapshot_00001.png
# ... etc
```

### Access Raw CSV
```bash
head 01_relaxation/output/disk_relaxation/run_2025-11-01_153056_DISPH_3d/outputs/csv/00000.csv
```

---

## 📖 Documentation

- **OUTPUT_GUIDE.md** - Comprehensive output documentation
- **README.md** - Workflow overview and usage
- **01_relaxation/README.md** - Lane-Emden physics documentation
- **workflow_viewer.py** - Interactive terminal viewer
- **visualizations/index.html** - Interactive web viewer

---

## ⚠️ Known Issues

1. **Timestep Explosion**: The relaxation simulation shows timestep explosion at snapshot 4, visible in evolution plots where the disk collapses from ~3 pc to ~0.05 pc radius.

2. **Physics Tuning Needed**: Consider adding timestep limiting or adjusting relaxation parameters for longer stable runs.

---

## 🚀 Next Steps

1. **Fix Physics**: Address timestep explosion in relaxation phase
2. **Run Step 2**: Execute flyby simulation with relaxed initial conditions
3. **Full Workflow**: Complete end-to-end 2-step workflow
4. **Comparison**: Run both DISPH and GDISPH methods

---

**Last Updated:** 2025-11-01  
**Location:** `/Users/guo/OSS/sphcode/simulations/workflows/razor_thin_hvcc_workflow`
