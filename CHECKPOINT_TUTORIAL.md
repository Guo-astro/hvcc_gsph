# Checkpoint/Resume Tutorial

This tutorial demonstrates how to use the checkpoint/resume feature in SPHCode to save and restore simulation state.

## Overview

The checkpoint system allows you to:
- **Auto-save** simulation state at regular intervals
- **Save on interrupt** (Ctrl+C) for graceful termination
- **Resume** simulations from saved checkpoints
- **Use checkpoints as initial conditions** for new runs

## Quick Start Guide

### Step 1: Enable Checkpointing

Add these fields to your simulation config JSON file:

```json
{
  "enableCheckpointing": true,
  "checkpointInterval": 2.0,
  "checkpointMaxKeep": 3,
  "checkpointOnInterrupt": true,
  "checkpointDirectory": "checkpoints"
}
```

**Field Descriptions:**
- `enableCheckpointing`: Turn on automatic checkpoint saving
- `checkpointInterval`: Save every N time units (e.g., 2.0 = save at t=2, 4, 6...)
- `checkpointMaxKeep`: Keep only the last N checkpoints (older ones auto-delete)
- `checkpointOnInterrupt`: Save checkpoint when you press Ctrl+C
- `checkpointDirectory`: Subdirectory name for checkpoints (default: "checkpoints")

### Step 2: Run Your Simulation

```bash
cd build
./sph1d <simulation_name> <config.json>
```

**What happens:**
- Simulation runs normally
- Checkpoints save automatically at specified intervals
- Output appears in `output/run_YYYYMMDD_HHMMSS/`
- Checkpoints appear in `output/run_YYYYMMDD_HHMMSS/checkpoints/`

**Example output:**
```
Saving checkpoint at t=2.000000
Checkpoint saved: output/run_20251101_120000/checkpoints/checkpoint_t2.000000.chk

Saving checkpoint at t=4.000000
Checkpoint saved: output/run_20251101_120000/checkpoints/checkpoint_t4.000000.chk
```

### Step 3: Interrupt with Ctrl+C (Optional)

While the simulation is running, press **Ctrl+C**:

```
^C
*** Interrupt signal received (Ctrl+C) ***
Saving interrupt checkpoint at t=3.456789
Checkpoint saved: output/run_20251101_120000/checkpoints/checkpoint_t3.456789.chk

To resume from this checkpoint, use:
  "resumeFromCheckpoint": true
  "resumeCheckpointFile": "output/run_20251101_120000/checkpoints/checkpoint_t3.456789.chk"

Exiting gracefully...
```

### Step 4: Resume from Checkpoint

Create a new config file (or modify the existing one) with these additions:

```json
{
  "resumeFromCheckpoint": true,
  "resumeCheckpointFile": "output/run_20251101_120000/checkpoints/checkpoint_t3.456789.chk",
  
  // Continue checkpointing after resume
  "enableCheckpointing": true,
  "checkpointInterval": 2.0,
  "checkpointMaxKeep": 3
}
```

**Important:** Use the **actual checkpoint path** from your run!

Then run with the resume config:

```bash
./sph1d <simulation_name> config_resume.json
```

**What happens:**
- Simulation loads particle state from checkpoint
- Time continues from where you left off (t=3.456789 in example)
- New checkpoints continue at the same intervals
- Output goes to a NEW run directory (preserves original data)

## Complete Example

Here's a complete workflow using the shock tube simulation:

### 1. Create checkpoint-enabled config

`config_checkpoint.json`:
```json
{
  "SPHType": "disph",
  "gamma": 1.4,
  "startTime": 0.0,
  "endTime": 0.2,
  "outputInterval": 0.02,
  
  "enableCheckpointing": true,
  "checkpointInterval": 0.05,
  "checkpointMaxKeep": 3,
  "checkpointOnInterrupt": true
}
```

### 2. Run simulation

```bash
./sph1d shock_tube config_checkpoint.json
```

Press Ctrl+C after a few seconds:

```
Time: t=0.123456
^C
*** Interrupt signal received (Ctrl+C) ***
Saving checkpoint at t=0.123456
Checkpoint saved: output/run_20251101_143000/checkpoints/checkpoint_t0.123456.chk
```

### 3. Create resume config

`config_resume.json`:
```json
{
  "SPHType": "disph",
  "gamma": 1.4,
  "startTime": 0.0,
  "endTime": 0.2,
  "outputInterval": 0.02,
  
  "resumeFromCheckpoint": true,
  "resumeCheckpointFile": "output/run_20251101_143000/checkpoints/checkpoint_t0.123456.chk",
  
  "enableCheckpointing": true,
  "checkpointInterval": 0.05,
  "checkpointMaxKeep": 3
}
```

### 4. Resume simulation

```bash
./sph1d shock_tube config_resume.json
```

Output:
```
Loading checkpoint from: output/run_20251101_143000/checkpoints/checkpoint_t0.123456.chk
Checkpoint loaded successfully
  Particles: 400
  Time: 0.123456
  Timestep: 0.000123
Resuming simulation from t=0.123456...

Time: t=0.150000
...
```

## Advanced Usage

### Use Checkpoint as Initial Conditions

You can use a checkpoint file to start a NEW simulation (not resume):

```json
{
  "initialConditionsFile": "output/run_xyz/checkpoints/checkpoint_t50.0.chk",
  "endTime": 100.0,
  "outputInterval": 1.0
}
```

This is useful for:
- Starting from a relaxed equilibrium state
- Running parameter studies from the same initial conditions
- Creating branching simulations

### Checkpoint File Format

Checkpoint files (`.chk`) contain:

```
[Header]
- Magic number: 0x53504843484B ("SPHCHK")
- Version: 1
- Timestamp
- Dimension (1D/2D/3D)
- Number of particles

[Parameters]
- JSON-encoded SPHParameters

[Particle Data]
- Binary array of all particle properties:
  * Position (x, y, z)
  * Velocity (vx, vy, vz)
  * Mass
  * Smoothing length
  * Density
  * Pressure
  * Internal energy
  * etc.

[Checksum]
- SHA-256 hash for integrity verification
```

### Checkpoint Cleanup

When `checkpointMaxKeep=3`, the system automatically deletes old checkpoints:

```
checkpoints/
  checkpoint_t2.0.chk      (deleted)
  checkpoint_t4.0.chk      (deleted)
  checkpoint_t6.0.chk      ← kept
  checkpoint_t8.0.chk      ← kept
  checkpoint_t10.0.chk     ← kept (newest)
```

This prevents disk space from filling up during long runs.

## Verification & Testing

### Test 1: Reproducibility

Run the same simulation twice and compare:

```bash
# Run 1: Continuous
./sph1d shock_tube config.json

# Run 2: With interruption
./sph1d shock_tube config.json
# Press Ctrl+C at t=0.1
./sph1d shock_tube config_resume.json

# Compare final outputs - should be identical!
```

### Test 2: Checkpoint Chain

Test multiple interruptions:

```bash
# Start → interrupt at t=0.05
./sph1d shock_tube config.json
^C

# Resume → interrupt at t=0.10
./sph1d shock_tube config_resume_1.json
^C

# Resume → complete to t=0.20
./sph1d shock_tube config_resume_2.json
```

### Test 3: Initial Conditions

Create relaxed initial state:

```bash
# Step 1: Relax particles
./sph1d disk_relax relax_config.json
# Runs to t=100, checkpoints at t=100

# Step 2: Use as initial conditions
./sph1d disk_evolve evolution_config.json
# Starts from relaxed state at t=100
```

## Troubleshooting

### Error: "Cannot open checkpoint file"

**Cause:** File path is incorrect

**Solution:** 
- Use absolute path or path relative to build directory
- Check that checkpoint file exists: `ls output/*/checkpoints/`
- Example: `"resumeCheckpointFile": "../output/run_20251101_120000/checkpoints/checkpoint_t5.0.chk"`

### Error: "Checkpoint validation failed"

**Cause:** File is corrupted (SHA-256 checksum mismatch)

**Solution:**
- Try a different checkpoint from the same run
- If all checkpoints fail, the original run may have had issues

### Error: "Dimension mismatch"

**Cause:** Checkpoint was saved with different spatial dimension

**Solution:**
- Check build dimension: `./sph1d` (1D), `./sph2d` (2D), `./sph3d` (3D)
- Rebuild project with correct dimension if needed
- Checkpoints are NOT portable across dimensions

### Warning: "Checkpoint overhead > 5%"

**Cause:** Checkpointing too frequently

**Solution:**
- Increase `checkpointInterval` (e.g., from 1.0 to 5.0)
- Reduce `checkpointMaxKeep` if you don't need many checkpoints
- For large simulations (>100K particles), checkpoint every 10-100 timesteps

## Performance Notes

**Typical Performance (10,000 particles):**
- Checkpoint save time: 50-100 ms
- Checkpoint load time: 20-50 ms
- File size: ~800 KB
- Overhead: <1% of total runtime

**For large simulations (1,000,000 particles):**
- Checkpoint save time: 2-5 seconds
- File size: ~80 MB
- Recommended interval: 10+ time units

**Best practices:**
- Balance checkpoint frequency vs. disk I/O overhead
- Use `checkpointMaxKeep` to limit disk usage
- For production runs, checkpoint every 5-10% of total simulation time

## Configuration Templates

### Minimal Checkpoint Config

```json
{
  "enableCheckpointing": true,
  "checkpointInterval": 5.0
}
```

### Production Config

```json
{
  "enableCheckpointing": true,
  "checkpointInterval": 10.0,
  "checkpointMaxKeep": 5,
  "checkpointOnInterrupt": true,
  "checkpointDirectory": "checkpoints"
}
```

### Resume Config

```json
{
  "resumeFromCheckpoint": true,
  "resumeCheckpointFile": "path/to/checkpoint.chk",
  "enableCheckpointing": true,
  "checkpointInterval": 10.0
}
```

### Initial Conditions from Checkpoint

```json
{
  "initialConditionsFile": "path/to/checkpoint.chk",
  "endTime": 200.0,
  "enableCheckpointing": true,
  "checkpointInterval": 20.0
}
```

## See Also

- **CHECKPOINT_README.md** - Quick reference guide
- **docs/CHECKPOINT_USER_GUIDE.md** - Comprehensive documentation
- **CHECKPOINT_COMPLETE_SUMMARY.md** - Technical implementation details
- **PHASE6_COMPLETE.md** - Signal handling (Ctrl+C) implementation

## Summary

✅ **Checkpointing** saves simulation state at regular intervals
✅ **Resume** continues from saved checkpoints
✅ **Interrupt handling** saves on Ctrl+C for graceful exits
✅ **Initial conditions** can be loaded from checkpoints
✅ **Automatic cleanup** prevents disk overflow

**Next steps:** Try it with your own simulations!
