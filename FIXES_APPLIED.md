# Fixes Applied - 2025-11-01

## Issue #1: Metadata JSON files output every frame ✅ FIXED

**Problem**: The CSV writer was creating a `.meta.json` file for every single snapshot (e.g., `00000.meta.json`, `00001.meta.json`, etc.), resulting in hundreds of metadata files.

**Root Cause**: The metadata writing code in `src/core/output_format.cpp` was not checking if metadata had already been written.

**Solution**: Modified `CSVOutputWriter::write_snapshot()` to only write metadata once on the first snapshot:

```cpp
// Write metadata JSON file (only once, for first snapshot)
if (m_snapshot_count == 0) {
    const std::string metadata_filename = m_directory + "/metadata.json";
    // ... write metadata ...
}
```

**Result**: Now only ONE `metadata.json` file is created in the CSV directory, containing column definitions and unit information.

**Files Modified**:
- `src/core/output_format.cpp` (lines 98-162)

**Testing**:
```bash
cd /Users/guo/OSS/sphcode/simulations/workflows/riemann_problems_workflow/01_simulation
export TEST_CASE=1
/Users/guo/OSS/sphcode/build/sph1d build/libriemann_plugin.dylib configs/test1_sod.json
ls results_test1_sod/riemann_problems/latest/outputs/csv/*.json
# Output: Only metadata.json (not 00000.meta.json, 00001.meta.json, etc.)
```

---

## Issue #2: No visualizations generated ⚠️ PARTIAL FIX

**Problem**: The `visualizations/` directory is created but remains empty after simulation completes.

**Root Cause**: There is no automatic post-processing step. The C++ code only creates the directory structure but doesn't call Python analysis scripts.

**Solution Option 1: Manual Analysis** ✅ WORKING
Run the analysis script manually after simulation:

```bash
cd /Users/guo/OSS/sphcode/simulations/workflows/shock_tube_workflow/01_simulation

# Run simulation
/Users/guo/OSS/sphcode/build/sph1d \
    build/libshock_tube_plugin.dylib \
    config/gdisph.json

# Generate visualizations manually
PYTHONPATH=/Users/guo/OSS/sphcode/analysis:$PYTHONPATH \
python scripts/compare_methods.py
```

**Solution Option 2: Wrapper Script** 🔧 CREATED (needs testing)
Created `run_with_analysis.sh` to automate the workflow:

```bash
cd /Users/guo/OSS/sphcode/simulations/workflows/shock_tube_workflow/01_simulation
./run_with_analysis.sh config/gdisph.json
```

This script:
1. Runs the simulation
2. Automatically calls the analysis script
3. Generates plots in the visualizations directory

**Files Created**:
- `simulations/workflows/shock_tube_workflow/01_simulation/run_with_analysis.sh`

**Files Modified**:
- `simulations/workflows/shock_tube_workflow/01_simulation/scripts/analyze_shock_tube.py` (fixed imports)

**Status**: Wrapper script created but analysis script needs debugging (dimension detection issue).

**Recommended Workflow**:
For now, use the manual two-step approach:
1. Run simulation
2. Run analysis script (compare_methods.py works correctly)

---

## Additional Notes

### Build System
- Had to rebuild `sph1d` and `sph2d` executables after code changes
- Used separate build directory: `build_manual/` with `-DBUILD_DIM=1`
- Copied updated executables to `build/` directory

### Metadata Format
The single `metadata.json` file contains:
```json
{
  "units": {
    "length": "cm",
    "time": "s",
    "mass": "g",
    ...
  },
  "simulation": {
    "time": 0.0,
    "dimension": 1,
    "particle_count": 400
  },
  "columns": [
    {"name": "time", "unit": "s", "description": "Simulation time"},
    {"name": "pos_x", "unit": "cm", "description": "X position"},
    ...
  ]
}
```

### Python Analysis Scripts
Working scripts in `simulations/workflows/shock_tube_workflow/01_simulation/scripts/`:
- `compare_methods.py` ✅ - Compares GDISPH vs SSPH  
- `quick_comparison.py` ✅ - Quick plot generation
- `analyze_shock_tube.py` ⚠️ - Needs dimension detection fix
- `animate_shock_tube.py` - Not tested

### Next Steps
1. Fix dimension detection in `SimulationReader` or pass dimension explicitly
2. Test `run_with_analysis.sh` wrapper script end-to-end
3. Consider adding automatic visualization generation to C++ code (call Python via system())
4. Update Riemann problems workflow to use same pattern

---

## Summary

| Issue | Status | Solution |
|-------|--------|----------|
| Metadata files every frame | ✅ **FIXED** | Modified output_format.cpp to write once |
| No visualizations | ⚠️ **WORKAROUND** | Use manual analysis or wrapper script |

**Both issues addressed with working solutions available!**
