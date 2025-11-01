# Makefile Guide for Razor-Thin HVCC GDISPH Workflow

This guide explains how to use the Makefile to manage the complete workflow.

## Table of Contents
- [Quick Start](#quick-start)
- [Common Tasks](#common-tasks)
- [All Available Targets](#all-available-targets)
- [Examples](#examples)
- [Troubleshooting](#troubleshooting)

## Quick Start

1. **Check dependencies**:
   ```bash
   make check-deps
   ```

2. **Run quick test workflow**:
   ```bash
   make workflow-test
   ```
   This runs a shortened version of the workflow to verify everything works.

3. **Run full workflow**:
   ```bash
   make workflow
   ```
   This runs the complete 2-step workflow with animation.

## Common Tasks

### Building

| Command | Description |
|---------|-------------|
| `make build-relax` | Build relaxation plugin (Step 1) |
| `make build-flyby` | Build flyby plugin (Step 2) |
| `make build-all` | Build both plugins |
| `make rebuild-all` | Clean and rebuild everything |

### Running Simulations

| Command | Description |
|---------|-------------|
| `make run-relax` | Run full relaxation (500 time units) |
| `make run-relax-test` | Run quick test (10 time units) |
| `make run-flyby` | Run flyby simulation |
| `make run-all` | Run both steps sequentially |

### Visualization

| Command | Description |
|---------|-------------|
| `make animate` | Create animation from existing data |
| `make quick-vis` | Quick snapshot visualization |

### Workflow Shortcuts

| Command | Description |
|---------|-------------|
| `make step1` | Run relaxation + animation |
| `make step1-test` | Quick relaxation test + animation |
| `make step2` | Run flyby simulation |
| `make workflow` | Complete workflow (all steps + animation) |
| `make workflow-test` | Quick workflow test |

### Cleaning

| Command | Description |
|---------|-------------|
| `make clean` | Clean outputs and animations (keep builds) |
| `make clean-output` | Clean only simulation outputs |
| `make clean-animations` | Clean only animations |
| `make clean-build` | Clean only build directories |
| `make clean-all` | Clean everything |

### Utilities

| Command | Description |
|---------|-------------|
| `make help` | Show all available commands |
| `make status` | Show current workflow status |
| `make check-deps` | Check dependencies |
| `make install-python-deps` | Install Python packages |

### Advanced

| Command | Description |
|---------|-------------|
| `make run-relax-bg` | Run relaxation in background with logging |
| `make stop-relax` | Stop background simulation |

## All Available Targets

Run `make help` to see all available targets with descriptions:

```bash
make help
```

## Examples

### Example 1: First-Time Setup and Quick Test

```bash
cd /Users/guo/OSS/sphcode/simulations/workflows/razor_thin_hvcc_gdisph_workflow

# Check dependencies
make check-deps

# Install Python dependencies if needed
make install-python-deps

# Run quick test workflow
make workflow-test

# Check what was created
make status
```

### Example 2: Full Production Run

```bash
cd /Users/guo/OSS/sphcode/simulations/workflows/razor_thin_hvcc_gdisph_workflow

# Run complete workflow
make workflow

# Output will be in:
# - 01_relaxation/output/
# - 02_flyby/output/
# - relaxation_animation.mp4
# - relaxation_comparison.png
```

### Example 3: Run Only Relaxation

```bash
# Run relaxation and create animation
make step1

# Or run quick test
make step1-test
```

### Example 4: Re-run with Fresh Start

```bash
# Clean everything
make clean-all

# Run workflow again
make workflow
```

### Example 5: Run Relaxation in Background

```bash
# Start relaxation in background
make run-relax-bg

# Monitor progress
tail -f 01_relaxation/logs/relax_*.log

# Stop if needed
make stop-relax

# After it completes, copy initial conditions and create animation
make copy-ic
make animate
```

### Example 6: Just Create Animation from Existing Data

```bash
# If you already have relaxation output, just create animation
make animate
```

### Example 7: Step-by-Step Workflow

```bash
# Step 1: Build
make build-all

# Step 2: Check status
make status

# Step 3: Run relaxation
make run-relax

# Step 4: Copy initial conditions
make copy-ic

# Step 5: Create animation
make animate

# Step 6: Run flyby
make run-flyby

# Step 7: Check final status
make status
```

## Troubleshooting

### Problem: "cmake not found" or "sph3d not found"

**Solution**: Build the main project first:
```bash
cd /Users/guo/OSS/sphcode/build
cmake .. -DDIM=3
make
```

### Problem: "matplotlib not found" or "pandas not found"

**Solution**: Install Python dependencies:
```bash
make install-python-deps
# or manually:
pip3 install matplotlib pandas numpy
```

### Problem: Animation creation fails with ffmpeg error

**Solution**: Install ffmpeg:
```bash
brew install ffmpeg
```

### Problem: "No relaxation runs found"

**Solution**: Run relaxation first:
```bash
make run-relax-test
```

### Problem: "Initial conditions not found" when running flyby

**Solution**: Complete Step 1 first or manually copy IC:
```bash
make run-relax-test
make copy-ic
```

### Problem: Build fails

**Solution**: Clean and rebuild:
```bash
make clean-build
make build-all
```

### Problem: Want to start completely fresh

**Solution**: Clean everything and rebuild:
```bash
make clean-all
make workflow-test
```

## Directory Structure

After running the workflow, you'll have:

```
razor_thin_hvcc_gdisph_workflow/
├── Makefile                          # This makefile
├── MAKEFILE_GUIDE.md                 # This guide
├── README.md                         # Workflow documentation
├── 01_relaxation/
│   ├── build/                        # Compiled plugin
│   ├── output/                       # Relaxation outputs
│   ├── logs/                         # Background run logs
│   ├── animate_relaxation.py         # Animation script
│   ├── relaxation_animation.mp4      # Generated animation
│   └── relaxation_comparison.png     # Generated plot
├── 02_flyby/
│   ├── build/                        # Compiled plugin
│   └── output/                       # Flyby outputs
└── initial_conditions/
    └── relaxed_disk.csv              # IC from Step 1 → Step 2
```

## Tips

1. **Always check status**: Use `make status` to see what's already done
2. **Use test versions**: Use `*-test` targets for quick verification
3. **Monitor background jobs**: Use `tail -f` to watch log files
4. **Clean selectively**: Use `make clean-output` to keep builds but reset data
5. **Check dependencies first**: Run `make check-deps` before long simulations

## Getting Help

- Run `make help` to see all commands
- Run `make status` to check current state
- Check the main README.md for workflow details
- See config.json files for simulation parameters
