# Razor-Thin HVCC GDISPH Workflow

This workflow performs a 2-step simulation of a razor-thin disk with an Intermediate Mass Black Hole (IMBH) flyby using the GDISPH SPH method for improved shock capturing.

## Workflow Structure

```
razor_thin_hvcc_gdisph_workflow/
├── 01_relaxation/          # Step 1: Generate relaxed disk
│   ├── disk_relaxation.cpp # Lane-Emden profile + relaxation
│   ├── config.json         # Relaxation parameters
│   ├── build.sh           # Build script
│   └── output/            # Relaxation outputs
├── 02_flyby/              # Step 2: IMBH flyby simulation
│   ├── flyby.cpp          # Flyby simulation code (GDISPH)
│   ├── config_gdisph.json # GDISPH flyby parameters
│   ├── build.sh           # Build script
│   └── output/            # Flyby simulation outputs
├── initial_conditions/    # Shared IC directory
│   └── relaxed_disk.csv  # Output from Step 1 → Input to Step 2
├── run_workflow.sh        # Automated workflow runner
└── README.md             # This file
```

## Quick Start

Run the complete workflow (relaxation + flyby):
```bash
cd /Users/guo/OSS/sphcode/simulations/workflows/razor_thin_hvcc_gdisph_workflow
./run_workflow.sh
```

## Workflow Steps

### Step 1: Disk Relaxation
- Generates initial disk particles using Lane-Emden profile
- Relaxes the disk to hydrostatic equilibrium
- Saves relaxed state to `initial_conditions/relaxed_disk.csv`

Run only Step 1:
```bash
./run_workflow.sh --step1-only
```

### Step 2: IMBH Flyby (GDISPH)
- Loads relaxed disk from Step 1
- Simulates IMBH flyby with GDISPH for better shock handling
- Outputs results to `02_flyby/output/`

Run only Step 2 (requires existing IC):
```bash
./run_workflow.sh --step2-only
```

## Options

```bash
./run_workflow.sh [OPTIONS]

Options:
  --step1-only    Run only Step 1 (relaxation)
  --step2-only    Run only Step 2 (flyby), requires existing IC
  --skip-build    Skip plugin rebuild if already built
  --help          Show help message
```

## Why GDISPH?

GDISPH (Generalized Density Independent SPH) provides:
- Better shock capturing than standard DISPH
- Improved handling of strong density gradients
- More accurate treatment of the tidal disruption region

## Output

- **Relaxation**: `01_relaxation/output/disk_relaxation/run_*/`
- **Flyby**: `02_flyby/output/run_*/`
- **Initial Conditions**: `initial_conditions/relaxed_disk.csv`

## Requirements

- SPH3D executable built in `/Users/guo/OSS/sphcode/build/`
- CMake for building plugins
- Python (optional, for analysis/visualization)

Use `./run_visualize.sh` to generate visualizations (requires python and dependencies).

