# Self-Contained Simulation Cases and Workflows

This directory contains both individual simulation cases and complete benchmark workflows.

## Structure Overview

### Individual Simulations
Each simulation case is completely self-contained with its own:
- Source code (`.cpp` files)
- Build configuration (`CMakeLists.txt`)
- Runtime configurations (`.json` files)
- Results (outputs from runs)

### Workflows
Complete benchmark workflows implement multiple related tests:
- Multiple test configurations
- Automated execution scripts
- Comprehensive analysis pipelines
- Comparison with analytical/theoretical solutions

## Directory Structure

```
simulations/
├── workflows/                           # Complete benchmark workflows
│   ├── shock_tube_workflow/            # GDISPH vs SSPH comparison
│   │   ├── README.md
│   │   ├── IMPLEMENTATION_COMPLETE.md
│   │   ├── 01_simulation/
│   │   │   ├── src/plugin.cpp
│   │   │   ├── build/libshock_tube_plugin.dylib
│   │   │   ├── configs/
│   │   │   │   ├── gdisph_config.json
│   │   │   │   └── ssph_config.json
│   │   │   ├── scripts/
│   │   │   │   └── compare_methods.py
│   │   │   └── results_*/
│   │   └── 02_analysis/
│   │
│   └── riemann_problems_workflow/      # 1D Riemann problems benchmark suite
│       ├── README.md                    # Workflow overview
│       ├── IMPLEMENTATION_COMPLETE.md   # Technical details
│       ├── RESULTS_SUMMARY.md           # Analysis of all test results
│       ├── EXECUTION_COMPLETE.md        # Execution summary
│       ├── 01_simulation/
│       │   ├── src/plugin.cpp           # Multi-test plugin
│       │   ├── build/libriemann_plugin.dylib
│       │   ├── configs/
│       │   │   ├── test1_sod.json
│       │   │   ├── test2_rarefaction.json
│       │   │   ├── test3_strong.json
│       │   │   └── test5_vacuum.json
│       │   ├── scripts/
│       │   │   └── analyze_all_tests.py
│       │   ├── results_test1_sod/
│       │   ├── results_test2_rarefaction/
│       │   ├── results_test3_strong/
│       │   ├── results_test5_vacuum/
│       │   └── comparison_results/      # Analysis plots
│       └── 02_analysis/
│
├── shock_tube/                          # Individual 1D Sod shock tube (legacy)
│   ├── shock_tube.cpp
│   ├── CMakeLists.txt
│   ├── config.json
│   └── ...
│
├── sedov_taylor/                        # 3D Sedov-Taylor (GSPH)
│   ├── sedov_taylor.cpp
│   └── ...
│
├── sedov_taylor_2d/                     # 2D Sedov-Taylor (DISPH)
│   ├── sedov_taylor_2d.cpp
│   ├── analytical/
│   │   └── sedov_taylor_solution.py
│   └── ...
│
└── template/                            # Template for new simulations
    └── ...
```

## Workflow Status

### Completed Workflows

#### 1. Shock Tube Workflow ✅
**Location**: `workflows/shock_tube_workflow/`  
**Purpose**: Compare GDISPH vs SSPH methods on classic Sod shock tube  
**Status**: VALIDATED  
**Results**:
- GDISPH L2 error: ~15.6%
- SSPH L2 error: ~17.4%
- Analysis complete with plots

**Key Files**:
- `01_simulation/src/plugin.cpp` - Sod shock tube initialization
- `01_simulation/scripts/compare_methods.py` - GDISPH vs SSPH comparison
- Fixed analytical solution (rarefaction fan formula)
- Fixed initial conditions (domain bug)

---

#### 2. Riemann Problems Workflow ⚠️
**Location**: `workflows/riemann_problems_workflow/`  
**Purpose**: Comprehensive 1D Riemann problems benchmark suite from DISPH paper  
**Status**: 4/5 TESTS COMPLETE  
**Tests Implemented**:
1. ✅ Sod Shock Tube (L2 error ~14%)
2. ✅ Double Rarefaction (L2 error ~62% - vacuum expected)
3. ❌ Strong Shock (UNSTABLE - needs shock limiter)
4. ⏭️ Slow Shock (not yet configured)
5. ✅ Vacuum Generation (L2 error ~36%)

**Results**: See `RESULTS_SUMMARY.md` for detailed analysis

**Key Features**:
- Multi-test plugin with `TEST_CASE` environment variable
- General Riemann solver in `analysis/theoretical.py`
- Automated analysis with 4-panel comparison plots
- CSV output with JSON metadata

**Known Issues**:
- Test 3: Pressure ratio 100,000:1 causes numerical instability
- Tests 2, 5: High errors expected due to vacuum formation (inherent SPH limitation)

**Next Steps**:
- Add Test 4 (Slow Shock)
- Implement shock limiter for Test 3
- Run GDISPH vs SSPH comparison on all tests

---

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
