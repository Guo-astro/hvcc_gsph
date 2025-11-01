# Checkpoint/Resume Test Simulation

This is a GDISPH version of the razor_thin_hvcc simulation, configured specifically for testing and demonstrating the checkpoint/resume functionality.

## Quick Start: Testing Checkpoint/Resume

### Step 1: Run Initial Simulation with Auto-Checkpoint

```bash
cd build
./sph3d razor_thin_hvcc_gdisph ../simulations/razor_thin_hvcc_gdisph/config.json
```

This will:
- Run the simulation from t=0 to t=10
- Save checkpoints automatically every 2.0 time units
- Create checkpoints at t=2, 4, 6, 8, 10
- Keep only the last 3 checkpoints

**Expected checkpoints** (in `output/run_YYYYMMDD_HHMMSS/checkpoints/`):
- `checkpoint_t6.000000.chk`
- `checkpoint_t8.000000.chk`
- `checkpoint_t10.000000.chk`

### Step 2: Test Interrupt with Ctrl+C

While the simulation is running, press **Ctrl+C** to interrupt:

```bash
./sph3d razor_thin_hvcc_gdisph ../simulations/razor_thin_hvcc_gdisph/config.json
# Press Ctrl+C after a few seconds
```

**Expected output**:
```
*** Interrupt signal received (Ctrl+C) ***
Saving interrupt checkpoint at t=3.456 to output/run_xyz/checkpoints/checkpoint_t3.456000.chk
Checkpoint saved successfully.
Resume with: "resumeFromCheckpoint": true, "resumeCheckpointFile": "output/run_xyz/checkpoints/checkpoint_t3.456000.chk"
Exiting gracefully...
```

### Step 3: Resume from Checkpoint

Update the checkpoint file path in `config_resume.json` with the actual checkpoint path from your run, then:

```bash
./sph3d razor_thin_hvcc_gdisph ../simulations/razor_thin_hvcc_gdisph/config_resume.json
```

This will:
- Load the simulation state from the checkpoint
- Continue from where it left off
- Keep checkpointing at the same intervals

**Verification**: Compare output files from continuous run vs resumed run - they should be identical!

## Configuration Options

### Auto-Checkpointing (config.json)

```json
{
  "enableCheckpointing": true,       // Enable auto-checkpoint
  "checkpointInterval": 2.0,         // Save every 2.0 time units
  "checkpointMaxKeep": 3,            // Keep last 3 checkpoints
  "checkpointOnInterrupt": true,     // Save on Ctrl+C
  "checkpointDirectory": "checkpoints"  // Subdirectory name
}
```

### Resume Configuration (config_resume.json)

```json
{
  "resumeFromCheckpoint": true,
  "resumeCheckpointFile": "path/to/checkpoint_tX.XXXXXX.chk",
  "enableCheckpointing": true,       // Continue checkpointing after resume
  "checkpointInterval": 2.0
}
```

## Checkpoint File Format

Checkpoint files (`.chk`) are binary files containing:
- **Header**: Magic number, version, timestamp, simulation metadata
- **Parameters**: JSON-encoded SPHParameters
- **Particle Data**: Binary dump of all particle states (position, velocity, mass, etc.)
- **Checksum**: SHA-256 hash for integrity verification

**Typical size**: ~1 MB per 10,000 particles

## Testing Scenarios

### Test 1: Continuous vs Resumed
1. Run simulation 0→10 continuously
2. Run simulation 0→5, save checkpoint
3. Resume from t=5 checkpoint to t=10
4. Compare final particle states - should be identical

### Test 2: Multiple Interrupts
1. Start simulation
2. Interrupt at t=2
3. Resume to t=5
4. Interrupt at t=7
5. Resume to t=10
6. Verify checkpoint chain works correctly

### Test 3: Checkpoint Cleanup
1. Run long simulation with `checkpointMaxKeep=3`
2. Verify old checkpoints are automatically deleted
3. Only last 3 checkpoints should exist

## Troubleshooting

### "Cannot find checkpoint file"
- Check the path in `resumeCheckpointFile`
- Use absolute path or path relative to build directory
- Checkpoint files are in `output/run_YYYYMMDD_HHMMSS/checkpoints/`

### "Checkpoint validation failed"
- File may be corrupted (SHA-256 mismatch)
- Try using a different checkpoint from the same run

### "Dimension mismatch"
- Checkpoint was saved with different DIM setting
- This simulation requires DIM=3

## Advanced: Creating Initial Conditions from Checkpoint

You can use checkpoints as initial conditions for new simulations:

```json
{
  "initialConditionsFile": "path/to/checkpoint_t50.chk",
  "endTime": 100.0
}
```

The simulation will:
1. Load particles from the checkpoint
2. Apply InitialConditionsModifier (if set)
3. Continue evolution from that state

This is useful for:
- Starting from relaxed equilibrium states
- Branching simulations from same initial state
- Parameter studies with consistent initial conditions

## Performance Notes

- **Checkpoint save**: <100ms for 10K particles
- **Checkpoint load**: <50ms for 10K particles
- **Overhead**: <1% impact on total simulation time

For very large simulations (>1M particles):
- Consider longer checkpoint intervals
- Monitor disk space usage
- Adjust `checkpointMaxKeep` based on available storage

## See Also

- **CHECKPOINT_COMPLETE_SUMMARY.md** - Full checkpoint system documentation
- **PHASE6_COMPLETE.md** - Signal handling implementation
- **README.md** - Main project README with checkpoint overview
