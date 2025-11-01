# Manual Checkpoint Tests

This directory contains scripts and configurations for manually testing the checkpoint/resume functionality.

## Files

- **test_checkpoint.sh** - Automated test script for checkpoint functionality
- **test_checkpoint_auto.json** - Sample configuration with auto-checkpoint enabled

## Usage

### Automated Test

From the `build/` directory:

```bash
cd build
../test/checkpoint_manual/test_checkpoint.sh
```

This will:
1. Create a test configuration
2. Run a simulation with auto-checkpoint
3. Verify checkpoints were created
4. Test resume from checkpoint

### Manual Test

1. **Run with checkpoint config**:
   ```bash
   cd build
   ./sph1d shock_tube ../test/checkpoint_manual/test_checkpoint_auto.json
   ```

2. **Check for checkpoints**:
   ```bash
   ls -lh output/run_*/checkpoints/
   ```

3. **Test resume** - Create a resume config pointing to a checkpoint, then:
   ```bash
   ./sph1d shock_tube path/to/resume_config.json
   ```

## Integration Tests

For comprehensive checkpoint unit tests, use:

```bash
cd build
ninja sph_unit_tests
./test/unit_tests/sph_unit_tests --gtest_filter="*Checkpoint*"
```

Result: 22/22 tests should pass.

## See Also

- **test/unit_tests/** - Automated checkpoint unit tests
- **docs/CHECKPOINT_USER_GUIDE.md** - Complete user guide
- **CHECKPOINT_README.md** - Quick reference
- **simulations/razor_thin_hvcc_gdisph/** - GDISPH checkpoint test simulation
