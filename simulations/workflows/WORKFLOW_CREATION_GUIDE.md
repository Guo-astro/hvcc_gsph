# Workflow Creation Guide

Complete guide for creating new SPH workflows using the scaffolding tool.

---

## Table of Contents

1. [Quick Start](#quick-start)
2. [Script Usage](#script-usage)
3. [Workflow Types](#workflow-types)
4. [Generated Structure](#generated-structure)
5. [Customization Guide](#customization-guide)
6. [Examples](#examples)
7. [Tips & Best Practices](#tips--best-practices)

---

## Quick Start

### Create a Simple Workflow

```bash
cd simulations/workflows
./create_workflow.sh my_simulation custom
```

This creates a complete workflow with:
- ✅ Professional folder structure
- ✅ Template C++ plugin file
- ✅ CMakeLists.txt for building
- ✅ Test and production configs
- ✅ README documentation
- ✅ Makefile for common tasks

### Build and Run

```bash
cd my_simulation_workflow
make build
make run-test
```

---

## Script Usage

### Basic Syntax

```bash
./create_workflow.sh <workflow_name> <workflow_type> [options]
```

### Arguments

| Argument | Required | Description |
|----------|----------|-------------|
| `workflow_name` | ✅ | Name of your workflow (e.g., `keplerian_disk`) |
| `workflow_type` | ✅ | Type of simulation (see below) |

### Options

| Option | Values | Default | Description |
|--------|--------|---------|-------------|
| `--dim=<N>` | 1, 2, 3 | 3 | Spatial dimension |
| `--sph-type=<type>` | gdisph, disph, ssph | gdisph | SPH algorithm |
| `--plugin-name=<name>` | any string | workflow_name | Custom plugin name |
| `--multi-step` | flag | false | Create multi-step workflow |

### Help

```bash
./create_workflow.sh --help
```

---

## Workflow Types

The script provides templates optimized for different physics:

| Type | Description | Use Case |
|------|-------------|----------|
| `custom` | Generic template | General-purpose, fully customizable |
| `collision` | Cloud-cloud collision | Molecular cloud collisions |
| `disk` | Disk relaxation | Accretion disks, planetary rings |
| `explosion` | Blast wave | Sedov-Taylor, supernovae |
| `shock` | Shock tube | Riemann problems, shock physics |
| `merger` | Binary merger | Star/planet mergers |

**Recommendation**: Start with `custom` for most cases. It provides complete structure with TODOs guiding implementation.

---

## Generated Structure

### Complete Workflow Directory

```
my_simulation_workflow/
├── README.md                     # Workflow overview
├── Makefile                      # Build automation
│
├── workflow_logs/                # 📋 Workflow execution logs
├── shared_data/                  # 🔄 Data shared between steps
├── workflow_results/             # 📊 Cross-step analysis
│   ├── animations/
│   ├── plots/
│   └── reports/
│
└── 01_simulation/                # Main simulation step
    ├── README.md                 # Step documentation
    ├── CMakeLists.txt           # Build configuration
    ├── .gitignore               # Git exclusions
    │
    ├── config/                  # Configuration files
    │   ├── production.json      # Full simulation
    │   └── test.json           # Quick test
    │
    ├── src/                     # Source code
    │   └── plugin.cpp          # Main plugin (CUSTOMIZABLE)
    │
    ├── scripts/                 # Analysis scripts
    ├── data/                    # Input data files
    ├── docs/                    # Documentation
    │
    ├── results/                 # Post-processed results
    │   ├── animations/
    │   ├── plots/
    │   └── analysis/
    │
    ├── output/                  # Raw simulation output (gitignored)
    └── build/                   # Build artifacts (gitignored)
```

### Multi-Step Workflow

With `--multi-step` flag:

```
my_simulation_workflow/
├── workflow_logs/
├── shared_data/
├── workflow_results/
├── 01_simulation/              # Step 1 (complete structure)
└── 02_simulation/              # Step 2 (complete structure)
```

---

## Customization Guide

### 1. Edit the Plugin Source

**Location**: `01_simulation/src/plugin.cpp`

The generated template includes:

```cpp
class MySimulationPlugin : public SimulationPlugin {
    // Plugin metadata
    std::string get_name() const override;
    std::string get_description() const override;
    
    // Main initialization
    void initialize(std::shared_ptr<Simulation> sim,
                   std::shared_ptr<SPHParameters> param) override {
        // TODO: Customize parameters
        // TODO: Initialize particles
        // TODO: Set initial conditions
    }
};
```

**Key customization points** (marked with `TODO`):

- **Simulation parameters**: Domain size, resolution
- **Physical parameters**: Density, pressure, temperature
- **SPH settings**: Viscosity, kernel type
- **Particle initialization**: Positions, velocities, properties
- **Initial conditions**: Perturbations, special setups

### 2. Adjust Configurations

**Files**: `config/production.json`, `config/test.json`

```json
{
  "time": {
    "endTime": 1.0,        // Adjust simulation duration
    "dtMax": 0.01,         // Maximum timestep
    "cflFactor": 0.3       // CFL safety factor
  },
  "output": {
    "snapshotInterval": 0.1  // Output frequency
  }
}
```

### 3. Update Documentation

Edit `README.md` files to document:
- Physics being simulated
- Expected results
- Configuration options
- References

---

## Examples

### Example 1: Simple 3D Explosion

```bash
./create_workflow.sh sedov_blast explosion --dim=3 --sph-type=gdisph
```

**Output**:
- Workflow: `sedov_blast_workflow/`
- Plugin: `sedov_blast`
- Dimension: 3D
- SPH Type: GDISPH

### Example 2: 2D Disk with Custom Name

```bash
./create_workflow.sh thin_disk disk --dim=2 --plugin-name=razor_thin
```

**Output**:
- Workflow: `thin_disk_workflow/`
- Plugin: `razor_thin` (custom)
- Dimension: 2D

### Example 3: Multi-Step Collision Workflow

```bash
./create_workflow.sh cloud_collision collision --multi-step
```

**Output**:
- Workflow: `cloud_collision_workflow/`
- Steps: `01_simulation/`, `02_simulation/`

### Example 4: 1D Shock Tube

```bash
./create_workflow.sh sod_shock shock --dim=1
```

**Output**:
- Workflow: `sod_shock_workflow/`
- Dimension: 1D
- Perfect for Riemann problems

---

## Tips & Best Practices

### Naming Conventions

✅ **Good names**:
- `keplerian_disk`
- `binary_star_merger`
- `molecular_cloud_collision`

❌ **Avoid**:
- `test123`
- `my_sim`
- Names with spaces or special characters

### Development Workflow

1. **Create** workflow with script
2. **Build** with `make build`
3. **Test** with `make run-test` (quick validation)
4. **Customize** plugin.cpp and configs
5. **Iterate** steps 2-4 until working
6. **Run** production with `make run-prod`

### Testing Strategy

```bash
# Quick test (low resolution, short time)
make run-test

# If test works, run production
make run-prod
```

**Test config** should be ~10-100x faster than production!

### Build Management

```bash
# Clean and rebuild
make clean
make build

# Force rebuild
cd 01_simulation
rm -rf build
cmake -B build -S .
cmake --build build
```

### Common Customizations

#### Change Particle Resolution

Edit `plugin.cpp`:
```cpp
const int particles_per_dim = 50;  // Low resolution
const int particles_per_dim = 100; // Medium resolution  
const int particles_per_dim = 200; // High resolution
```

#### Adjust Output Frequency

Edit `config/*.json`:
```json
"output": {
  "snapshotInterval": 0.01  // More frequent
}
```

#### Enable Gravity

Edit `plugin.cpp`:
```cpp
param->gravity_enabled = true;
param->gravitational_constant = 1.0;
```

### Adding More Steps

To add a third step to existing workflow:

```bash
cd my_workflow
cp -r 01_simulation 03_analysis
# Edit 03_analysis as needed
```

### Version Control

The script creates `.gitignore` files to exclude:
- `build/` - Build artifacts
- `output/` - Large simulation data

**Commit** your workflow:
```bash
cd my_simulation_workflow
git add .
git commit -m "Add my_simulation workflow"
```

### Integration with Analysis

```python
# In scripts/analyze.py
import sys
sys.path.append('../../analysis')
from readers import read_snapshot

# Read from workflow output
data = read_snapshot('../output/my_simulation/snapshot_0001.csv')
```

---

## Troubleshooting

### Build Errors

**Problem**: `CMake Error: SPH_ROOT not found`

**Solution**: Check CMakeLists.txt paths - adjust relative path to SPH root:
```cmake
get_filename_component(SPH_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../../../" ABSOLUTE)
```

### Runtime Errors

**Problem**: `Cannot load plugin`

**Solution**: Check that dimension matches:
```cpp
#if DIM != 3
    THROW_ERROR("Plugin requires DIM=3");
#endif
```

### Plugin Not Found

**Problem**: `make run-test` says "plugin not found"

**Solution**: Build first!
```bash
make build
ls 01_simulation/build/*.dylib  # Should exist
```

---

## Advanced Usage

### Custom Plugin Template

To add your own template type:

1. Edit `create_workflow.sh`
2. Add function `create_mytype_template()`
3. Add case in template selection

### Workflow Variants

Create related workflows:
```bash
./create_workflow.sh disk_low_res disk --dim=2
./create_workflow.sh disk_high_res disk --dim=2 --plugin-name=disk_hires
```

### Batch Creation

```bash
# Create multiple test workflows
for type in collision disk explosion shock; do
    ./create_workflow.sh test_${type} ${type}
done
```

---

## Related Documentation

- **[FOLDER_STRUCTURE_EXPLANATION.md](FOLDER_STRUCTURE_EXPLANATION.md)** - Folder organization details
- **[WORKFLOW_STRUCTURE_TEMPLATE.md](WORKFLOW_STRUCTURE_TEMPLATE.md)** - Standard structure reference
- **[FOLDER_NAMING_IMPROVEMENTS.md](FOLDER_NAMING_IMPROVEMENTS.md)** - Naming rationale

---

## Quick Reference

```bash
# Create workflow
./create_workflow.sh NAME TYPE [OPTIONS]

# Build
cd NAME_workflow && make build

# Test
make run-test

# Run
make run-prod

# Clean
make clean
```

**Common commands**:
```bash
# 3D GDISPH custom
./create_workflow.sh my_sim custom

# 2D disk
./create_workflow.sh my_disk disk --dim=2

# Multi-step
./create_workflow.sh my_workflow custom --multi-step

# 1D DISPH shock
./create_workflow.sh shock_1d shock --dim=1 --sph-type=disph
```

---

**Ready to create your first workflow? Start here:**

```bash
cd /Users/guo/OSS/sphcode/simulations/workflows
./create_workflow.sh --help
```

🚀 Happy simulating!
