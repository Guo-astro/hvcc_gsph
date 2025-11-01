# Quick Start: New Simulation Run System

## Overview

The SPH code now uses an organized, self-contained directory structure for all simulation runs. Every run automatically creates:

```
simulations/{sample_name}/{run_id}/
├── metadata.json              # Complete run information
├── config.json                # Exact configuration used
├── initial_conditions.csv     # Initial conditions for reproducibility
├── outputs/
│   ├── csv/                   # CSV format outputs
│   └── binary/                # Binary format outputs (faster, smaller)
├── visualizations/            # Your plots and animations
├── logs/                      # Execution logs
└── analysis/                  # Analysis results
```

## Benefits

✅ **Complete Reproducibility** - Everything needed to reproduce the run in one place  
✅ **Version Tracking** - Git hash, compiler version, exact parameters saved  
✅ **Easy Archiving** - Single directory contains all data  
✅ **Quick Access** - `latest` symlink points to most recent run  
✅ **Multiple Formats** - Save CSV and binary simultaneously  

## Example: Running Shock Tube

### 1. Update Your Config (Optional)

The old configs still work! But for the new features, add:

```json
{
  "sampleName": "shock_tube",
  "description": "Standard Sod shock tube test",
  "output": {
    "baseDirectory": "simulations",
    "formats": ["csv", "binary"],
    "saveInitialConditions": true,
    "saveMetadata": true
  },
  "SPHType": "disph",
  "endTime": 0.2,
  "outputInterval": 0.02
}
```

### 2. Run Simulation

```bash
cd /Users/guo/OSS/sphcode/build
./sph1d shock_tube ../configs/benchmarks/shock_tube.json
```

### 3. Check Output

```bash
ls simulations/shock_tube/latest/
# metadata.json  config.json  initial_conditions.csv  outputs/  visualizations/  logs/  analysis/

ls simulations/shock_tube/latest/outputs/
# csv/  binary/
```

## Python Analysis

### Load Latest Run

```python
from analysis.simulation_runs import load_latest

# Load latest shock tube run
run = load_latest("shock_tube")
print(run)

# Get binary output reader (fast!)
reader = run.get_output_reader("binary")
data = reader.read_snapshot(0)
print(data['dens'])

# Check metadata
print(f"Git hash: {run.metadata['code_version']['git_hash']}")
print(f"Particles: {run.metadata['simulation_params']['particle_count']}")
```

### Find All Runs

```python
from analysis.simulation_runs import SimulationRunFinder

finder = SimulationRunFinder()

# List all samples
samples = finder.list_samples()
print(samples)  # ['shock_tube', 'sedov_taylor', ...]

# Get all shock tube runs
runs = finder.find_all("shock_tube")
for run in runs:
    info = run.get_info()
    print(f"{info.run_id}: {info.particle_count} particles, {info.git_hash}")
```

### Compare Runs

```python
from analysis.simulation_runs import compare_runs

run1 = finder.find_all("shock_tube")[0]
run2 = finder.find_all("shock_tube")[1]

diff = compare_runs(run1, run2)
print("Differences:", diff)
```

## Command Line Tools

### List All Runs

```bash
python3 analysis/simulation_runs.py --summary
# Simulation Runs Summary:
#   Total samples: 3
#   Total runs: 15
# Per sample:
#   shock_tube: 5 run(s)
#   sedov_taylor: 8 run(s)
#   evrard: 2 run(s)
```

### Check Latest Run

```bash
python3 analysis/simulation_runs.py shock_tube
# SimulationRun(run_id='run_2025-11-01_143052_disph_1d', ...)
# Run Info:
#   Sample: shock_tube
#   Created: 2025-11-01T14:30:52Z
#   Method: DISPH (1D)
#   Particles: 500
#   Snapshots: 11
#   Git Hash: a3f2b1c
```

### Inspect Specific Run

```bash
python3 analysis/simulation_runs.py simulations/shock_tube/run_2025-11-01_143052_disph_1d
```

## Migration from Old System

### Old Way (Still Works)

```json
{
  "outputDirectory": "results/DISPH/shock_tube/1D"
}
```

**Outputs to:** `build/results/DISPH/shock_tube/1D/`

### New Way (Recommended)

```json
{
  "sampleName": "shock_tube",
  "output": {
    "autoRunId": true
  }
}
```

**Outputs to:** `simulations/shock_tube/run_2025-11-01_143052_disph_1d/outputs/csv/`

## Archive a Run

```bash
# Create archive with everything
cd simulations/shock_tube
tar -czf shock_tube_$(date +%Y%m%d).tar.gz latest/

# Share with collaborators - they get:
# - Exact code version (git hash)
# - Exact configuration
# - Initial conditions
# - All outputs
# - Performance metrics
```

## Tips

1. **Use Binary Format** for large runs (9x smaller, 20x faster)
2. **Check metadata.json** to verify run parameters
3. **Use `latest` symlink** for quick access in scripts
4. **Tag important runs** by copying to `simulations/production/`
5. **Compare runs** using metadata diff

## Troubleshooting

### No `latest` symlink?

Check if runs exist:
```bash
ls simulations/shock_tube/
```

Create manually if needed:
```bash
cd simulations/shock_tube
ln -s run_2025-11-01_143052_disph_1d latest
```

### Old output directory still used?

The code uses the new system automatically. Check your JSON doesn't explicitly set:
```json
"outputDirectory": "results/..."  # Remove this for new system
```

### Can't find runs in Python?

Check base directory:
```python
from pathlib import Path
print(Path("simulations").exists())  # Should be True
```

## See Also

- `docs/architecture/SIMULATION_RUN_STRUCTURE.md` - Complete structure documentation
- `docs/user_guide/OUTPUT_FORMATS.md` - Format comparison and performance
- `analysis/simulation_runs.py` - Python tools source code
