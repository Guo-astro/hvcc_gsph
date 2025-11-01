# Self-Contained Simulation Cases

Each simulation case is completely self-contained with its own:
- Source code (`.cpp` files)
- Build configuration (`CMakeLists.txt`)
- Runtime configurations (`.json` files)
- Results (outputs from runs)

## Directory Structure

```
simulations/
├── shock_tube/                          # 1D Sod shock tube (DISPH)
│   ├── shock_tube.cpp                   # Source code defining the simulation
│   ├── CMakeLists.txt                   # Build file for this specific case
│   ├── config.json                      # Default configuration
│   ├── build.sh                         # Build script
│   ├── README.md                        # Documentation for this case
│   ├── analysis/                        # Simulation-specific analysis
│   │   └── shock_tube_tutorial.ipynb   # Jupyter tutorial
│   ├── latest -> run_2025-11-01_...    # Symlink to latest run
│   └── run_2025-11-01_095541_DISPH_1d/ # Individual run results
│       ├── metadata.json
│       ├── config.json
│       ├── initial_conditions.csv
│       ├── source/                      # Source code snapshot
│       │   └── shock_tube.cpp
│       ├── outputs/                     # CSV and binary snapshots
│       │   ├── csv/
│       │   └── binary/
│       ├── analysis/                    # Analysis results
│       ├── visualizations/              # Plots and animations
│       └── logs/                        # Simulation logs
│
├── sedov_taylor/                        # 3D Sedov-Taylor (GSPH)
│   ├── sedov_taylor.cpp
│   ├── CMakeLists.txt
│   ├── config.json
│   ├── sedov_taylor.json               # Alternative config (from benchmarks)
│   └── ...
│
├── sedov_taylor_2d/                     # 2D Sedov-Taylor (DISPH)
│   ├── sedov_taylor_2d.cpp
│   ├── CMakeLists.txt
│   ├── config.json
│   ├── analytical/                      # Analytical solutions
│   │   └── sedov_taylor_solution.py    # Self-similar solution (2D/3D)
│   └── ...
│
└── template/                            # Template for new simulations
    ├── template_simulation.cpp
    ├── CMakeLists.txt
    └── README.md
```

## How It Works

### 1. Building a Simulation Plugin

Each simulation compiles to a shared library (plugin):

```bash
cd simulations/shock_tube
mkdir build
cd build
cmake ..
make
# Creates: libshock_tube_plugin.so (or .dylib on macOS)
```

### 2. Running a Simulation

The main executable loads the plugin dynamically:

```bash
cd /path/to/sphcode/build
./sph1d ../simulations/shock_tube/build/libshock_tube_plugin.so
```

Or simpler wrapper:

```bash
./run_simulation shock_tube
```

### 3. Self-Contained Runs

Every run copies the source files used:

```
run_2025-11-01_095541/
├── shock_tube.cpp          # Exact source code
├── CMakeLists.txt          # Exact build config
├── metadata.json           # Git hash, compile flags, etc.
└── outputs/                # Results
```

## Benefits of This Organization

✅ **Single source of truth**: Each simulation is self-contained  
✅ **Complete reproducibility**: Source code + config + outputs all together  
✅ **Easy collaboration**: Share entire simulation directory  
✅ **Analytical solutions included**: Compare with exact solutions  
✅ **Analysis tools co-located**: Notebooks and scripts with the simulation  
✅ **No src/samples**: Main codebase stays clean  
✅ **Independent builds**: Each case builds independently  
✅ **Hot reload**: Recompile plugin without rebuilding main executable  

## Reorganization from Previous Structure

**Configs moved**:
- `/configs/benchmarks/sedov_taylor.json` → `simulations/sedov_taylor/`
- Base configs remain in `/configs/base/` for reference

**Analysis moved**:
- `/analysis/examples/shock_tube_tutorial.ipynb` → `simulations/shock_tube/analysis/`
- General analysis tools remain in `/analysis/` (readers, plotting, etc.)
- Simulation-specific notebooks now live with their simulations

**Analytical solutions added**:
- `simulations/sedov_taylor_2d/analytical/sedov_taylor_solution.py` - Self-similar blast wave solution
- Can compare simulation vs analytical results directly

## Creating a New Simulation

```bash
# 1. Copy template
cp -r simulations/template simulations/my_new_case

# 2. Edit the source
cd simulations/my_new_case
# Edit my_new_case.cpp with your initial conditions

# 3. Build the plugin
mkdir build && cd build
cmake ..
make

# 4. Run it
cd /path/to/sphcode/build
./run_simulation my_new_case
```

## Migration from Old System

Old `src/samples/` → New `simulations/` mapping:
- `src/samples/benchmarks/shock_tubes/sod_shock_tube.cpp` → `simulations/shock_tube/shock_tube.cpp`
- `src/samples/benchmarks/sedov_taylor/sedov_taylor.cpp` → `simulations/sedov_taylor/sedov_taylor.cpp`
- etc.

Each case becomes a standalone, buildable simulation.
