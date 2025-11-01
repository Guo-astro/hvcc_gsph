# Checkpoint/Resume System - User Guide

**Quick Start Guide** for the SPHCode Checkpoint/Resume System

---

## Overview

The checkpoint system allows you to:
- **Pause** long-running simulations and resume later
- **Auto-save** simulation state at regular intervals
- **Safely interrupt** simulations with Ctrl+C
- **Branch** simulations from saved states

---

## Quick Examples

### 1. Basic Auto-Checkpoint

Add to your `config.json`:
```json
{
  "endTime": 100.0,
  "enableCheckpointing": true,
  "checkpointInterval": 10.0,
  "checkpointMaxKeep": 3
}
```

Run:
```bash
./build/sph1d shock_tube sample/shock_tube/shock_tube.json
```

**Result**: Checkpoints saved at t=10, 20, 30, ..., 100 (keeps last 3)

---

### 2. Interrupt with Ctrl+C

Start simulation:
```bash
./build/sph1d shock_tube sample/shock_tube/shock_tube.json
```

Press **Ctrl+C** while running:
```
*** Interrupt signal received (Ctrl+C) ***
Saving interrupt checkpoint at t=47.3 to output/run_20251101_143022/checkpoints/checkpoint_t47.300000.chk
Checkpoint saved successfully.
Resume with: "resumeFromCheckpoint": true, "resumeCheckpointFile": "output/run_20251101_143022/checkpoints/checkpoint_t47.300000.chk"
```

---

### 3. Resume from Checkpoint

Create `resume.json`:
```json
{
  "resumeFromCheckpoint": true,
  "resumeCheckpointFile": "output/run_20251101_143022/checkpoints/checkpoint_t47.300000.chk",
  "endTime": 100.0,
  "enableCheckpointing": true,
  "checkpointInterval": 10.0
}
```

Resume:
```bash
./build/sph1d shock_tube resume.json
```

**Result**: Continues from t=47.3 to t=100

---

## Configuration Reference

### Checkpoint Save Options

```json
{
  "enableCheckpointing": true,        // Enable auto-checkpoint
  "checkpointInterval": 5.0,          // Save every 5.0 time units
  "checkpointMaxKeep": 5,             // Keep last 5 checkpoints
  "checkpointOnInterrupt": true,      // Save on Ctrl+C
  "checkpointDirectory": "checkpoints" // Directory name (relative to output)
}
```

**Default values**:
- `enableCheckpointing`: `false`
- `checkpointInterval`: `1.0`
- `checkpointMaxKeep`: `3`
- `checkpointOnInterrupt`: `true`
- `checkpointDirectory`: `"checkpoints"`

### Resume Options

```json
{
  "resumeFromCheckpoint": true,
  "resumeCheckpointFile": "path/to/checkpoint.chk"
}
```

**Notes**:
- Path can be absolute or relative to build directory
- Must specify exact checkpoint file (not directory)
- Simulation will continue from saved time and state

---

## Common Use Cases

### Long Overnight Simulations

**Problem**: Simulation takes 48 hours, but cluster has 24-hour wall time limit.

**Solution**:
```json
{
  "endTime": 200.0,
  "enableCheckpointing": true,
  "checkpointInterval": 20.0
}
```

1. Run for 24 hours (reaches t≈100)
2. Job terminated, latest checkpoint at t=100
3. Resume with `resumeCheckpointFile` pointing to t=100 checkpoint
4. Complete simulation to t=200

### Parameter Studies

**Problem**: Need to run 10 simulations with different parameters from same initial state.

**Solution**:
1. Run initial relaxation to equilibrium (t=50)
2. Save checkpoint at t=50
3. Create 10 different resume configs with different parameters
4. All simulations start from identical t=50 state

### Testing and Debugging

**Problem**: Need to test code changes on specific simulation state.

**Solution**:
1. Run simulation with checkpointing
2. Interrupt at interesting time (e.g., during shock)
3. Make code changes
4. Resume from checkpoint to test new code on same state

### Exploratory Science

**Problem**: Want to explore simulation evolution interactively.

**Solution**:
```json
{
  "checkpointInterval": 1.0,
  "checkpointOnInterrupt": true
}
```

1. Run simulation
2. Interrupt when you see interesting feature
3. Analyze checkpoint data
4. Resume to continue or branch to new parameters

---

## File Locations

### Checkpoint Directory Structure

```
output/
└── run_20251101_143022/         # Run directory
    ├── checkpoints/              # Checkpoint subdirectory
    │   ├── checkpoint_t10.000000.chk
    │   ├── checkpoint_t20.000000.chk
    │   └── checkpoint_t30.000000.chk
    ├── snapshots/                # Regular output files
    │   ├── 00001.bin
    │   ├── 00002.bin
    │   └── ...
    └── metadata.json
```

### Checkpoint File Naming

Format: `checkpoint_tTIME.chk`
- `TIME` = simulation time with 6 decimal places
- Example: `checkpoint_t47.300000.chk` = checkpoint at t=47.3

---

## Checkpoint File Format

Binary format optimized for performance:

```
[Header: 512 bytes]
  - Magic: "SPHCHKPT"
  - Version: 1
  - Timestamp: ISO 8601
  - Simulation metadata

[Parameters: JSON]
  - SPHParameters as JSON

[Particle Data: Binary]
  - All particle states
  - Position, velocity, mass, etc.

[Checksum: 32 bytes]
  - SHA-256 hash for integrity
```

**Size**: Approximately 1 MB per 10,000 particles

---

## Best Practices

### ✅ DO

- **Set reasonable intervals**: Balance overhead vs recovery granularity
  - Short simulations (t<10): interval = 1.0
  - Medium simulations (t=10-100): interval = 5.0
  - Long simulations (t>100): interval = 10.0+

- **Enable interrupt checkpoints**: Always use `checkpointOnInterrupt: true`

- **Monitor disk space**: Each checkpoint can be large for big simulations
  - 10K particles ≈ 1 MB
  - 100K particles ≈ 10 MB
  - 1M particles ≈ 100 MB

- **Test resume before long runs**: Verify checkpoint/resume works for your simulation

### ❌ DON'T

- **Don't rely on single checkpoint**: Use `checkpointMaxKeep >= 2` in case of corruption

- **Don't delete checkpoint directory**: During simulation - files are cleaned automatically

- **Don't manually edit checkpoints**: Binary format - any modification breaks SHA-256 checksum

- **Don't change dimensions**: Cannot resume 2D checkpoint in 3D build (and vice versa)

---

## Troubleshooting

### Error: "Cannot find checkpoint file"

**Cause**: Wrong path or file doesn't exist

**Fix**: 
```bash
# Check checkpoint exists
ls -la output/run_*/checkpoints/

# Use absolute path
"resumeCheckpointFile": "/full/path/to/checkpoint_t10.000000.chk"
```

### Error: "Checkpoint validation failed"

**Cause**: File corrupted (SHA-256 mismatch)

**Fix**: Use a different checkpoint from same run
```bash
# List available checkpoints
ls -lt output/run_*/checkpoints/
# Pick another checkpoint
```

### Error: "Dimension mismatch"

**Cause**: Checkpoint saved with different DIM setting

**Fix**: Build correct dimension
```bash
# If checkpoint is 3D
cmake -B build -DBUILD_DIM=3
cmake --build build
./build/sph3d simulation config_resume.json
```

### Simulation crashes after resume

**Possible causes**:
1. **Corrupted checkpoint**: Try different checkpoint
2. **Changed parameters**: Some parameters cannot change during resume
3. **Code changes**: Incompatible changes in particle structure

**Debug**:
```bash
# Verify checkpoint is loadable
./build/sph1d simulation config_resume.json 2>&1 | head -50

# Check for error messages in output
```

---

## Performance Impact

Checkpoint system is designed for minimal overhead:

| Operation | Time (10K particles) | Overhead |
|-----------|---------------------|----------|
| Checkpoint save | ~50 ms | <0.1% per save |
| Checkpoint load | ~30 ms | One-time at startup |
| Interrupt check | <1 μs | <0.01% continuous |

**Recommendation**: Even with `checkpointInterval=1.0`, total overhead <1% for typical simulations.

---

## Advanced: Using Checkpoints as Initial Conditions

Checkpoints can be used as initial conditions for new simulations:

```json
{
  "initialConditionsFile": "path/to/checkpoint_t50.chk",
  "endTime": 100.0
}
```

**Use cases**:
- Start from relaxed equilibrium
- Continue simulation with different physics
- Apply InitialConditionsModifier to checkpoint data

**Example**: Load equilibrium disk, add IMBH, run flyby
```cpp
// In simulation plugin
sim->set_initial_conditions_modifier(
    std::make_shared<InfallModifier>(target, speed, mass)
);
```

See `simulations/razor_thin_hvcc_gdisph/` for complete example.

---

## Testing Checkpoint Functionality

A test simulation is provided: **razor_thin_hvcc_gdisph**

```bash
cd build

# Step 1: Run with auto-checkpoint
./sph3d razor_thin_hvcc_gdisph ../simulations/razor_thin_hvcc_gdisph/config.json

# Step 2: Interrupt with Ctrl+C after a few seconds

# Step 3: Update config_resume.json with checkpoint path

# Step 4: Resume
./sph3d razor_thin_hvcc_gdisph ../simulations/razor_thin_hvcc_gdisph/config_resume.json
```

See `simulations/razor_thin_hvcc_gdisph/README.md` for detailed test instructions.

### Automated Testing

Run the automated test script:

```bash
cd build
../test/checkpoint_manual/test_checkpoint.sh
```

Or run unit tests:

```bash
cd build
ninja sph_unit_tests
./test/unit_tests/sph_unit_tests --gtest_filter="*Checkpoint*"
```

**Result**: 22/22 tests passing ✅

---

## Summary

**Essential config for most users**:
```json
{
  "enableCheckpointing": true,
  "checkpointInterval": 5.0,
  "checkpointMaxKeep": 3,
  "checkpointOnInterrupt": true
}
```

**To resume**:
```json
{
  "resumeFromCheckpoint": true,
  "resumeCheckpointFile": "output/run_xyz/checkpoints/checkpoint_tX.chk"
}
```

**That's it!** The checkpoint system handles everything else automatically.

---

## Further Reading

- **CHECKPOINT_COMPLETE_SUMMARY.md** - Complete technical documentation
- **PHASE5_COMPLETE.md** - Integration details
- **PHASE6_COMPLETE.md** - Signal handling implementation
- **simulations/razor_thin_hvcc_gdisph/README.md** - Testing guide

For questions or issues, see project documentation or contact maintainers.
