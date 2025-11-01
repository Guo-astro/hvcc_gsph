# Behavior-Driven Development Tests for SPH Simulation Workflow

## Feature: CSV Metadata Output

**As a** simulation user  
**I want** metadata to be written only once per simulation run  
**So that** I don't have hundreds of redundant metadata files cluttering my output directory

### Scenario: Single metadata file is generated
**Given** a clean output directory  
**When** I run a 1D Riemann problem simulation with 50 timesteps  
**Then** exactly 1 metadata.json file should exist in the CSV output directory  
**And** the metadata.json file should be at the root of the CSV directory  
**And** there should be no files matching the pattern `*.meta.json`  
**And** all CSV files should still be generated correctly

### Scenario: Metadata contains correct information
**Given** a completed simulation run  
**When** I read the metadata.json file  
**Then** it should contain a "units" section with all physical units  
**And** it should contain a "simulation" section with dimension and particle count  
**And** it should contain a "columns" array describing all CSV columns  
**And** each column should have name, unit, and description fields

### Scenario: Metadata file location is consistent
**Given** multiple simulation runs  
**When** I check each run's output directory  
**Then** the metadata.json should always be at `<run_dir>/outputs/csv/metadata.json`  
**And** not at `<run_dir>/outputs/csv/00000.meta.json`

---

## Feature: Automatic Visualization Generation

**As a** simulation user  
**I want** visualizations to be automatically generated after each simulation  
**So that** I can immediately see the results without manual post-processing

### Scenario: Initial conditions visualization is generated
**Given** a completed Riemann problem simulation  
**When** automatic visualization runs  
**Then** an "initial_conditions.png" file should exist in the visualizations directory  
**And** it should be a 4-panel plot showing density, velocity, pressure, and energy  
**And** all plots should have correct physical units in axis labels  
**And** the plot should show both SPH particles and analytical solution

### Scenario: Final state visualization is generated
**Given** a completed Riemann problem simulation  
**When** automatic visualization runs  
**Then** a "final_state.png" file should exist in the visualizations directory  
**And** it should be a 4-panel plot showing density, velocity, pressure, and energy  
**And** all plots should have correct physical units in axis labels  
**And** the time stamp should match the final simulation time

### Scenario: Key timestep screenshots are generated
**Given** a completed Riemann problem simulation with N timesteps  
**When** automatic visualization runs  
**Then** at least 3 screenshot files should exist (25%, 50%, 75% of simulation time)  
**And** each screenshot should show all 4 physics variables with units  
**And** file names should contain the timestamp for identification

### Scenario: Animation is generated
**Given** a completed Riemann problem simulation  
**When** automatic visualization runs  
**Then** an "evolution_animation.mp4" or "evolution_animation.gif" file should exist  
**And** the animation should contain frames from initial to final state  
**And** each frame should show all 4 physics variables

### Scenario: Visualization directory structure is correct
**Given** a simulation with output directory "results_test1_sod"  
**When** visualization completes  
**Then** visualizations should be in "results_test1_sod/riemann_problems/latest/visualizations/"  
**And** not in "results_test1_sod/" or "comparison_results/"

### Scenario: Visualization generation is resilient to interruption
**Given** a simulation that is interrupted mid-run  
**When** automatic visualization attempts to run  
**Then** it should generate visualizations for all available timesteps  
**And** it should not crash due to missing final timesteps  
**And** it should log a warning about incomplete data

### Scenario: Physical units are correctly displayed
**Given** a simulation with custom unit system (e.g., CGS)  
**When** visualizations are generated  
**Then** density plots should show units like "g/cm³"  
**And** pressure plots should show units like "dyne/cm²"  
**And** velocity plots should show units like "cm/s"  
**And** energy plots should show units like "erg/g"  
**And** position axes should show units like "cm"

---

## Feature: Workflow Integration

**As a** simulation user  
**I want** the entire workflow to run automatically from a single command  
**So that** I can get complete results without manual intervention

### Scenario: Complete workflow runs successfully
**Given** I have a Riemann problem configuration file  
**When** I run `./run_with_visualization.sh 1 configs/test1_sod.json`  
**Then** the simulation should complete without errors  
**And** visualizations should be automatically generated  
**And** the final output directory should contain CSV data and visualizations  
**And** a success message should be displayed

### Scenario: Workflow handles simulation errors gracefully
**Given** a configuration file with invalid parameters  
**When** I run the workflow script  
**Then** the simulation should fail with a clear error message  
**And** visualization generation should be skipped  
**And** the exit code should be non-zero

### Scenario: Workflow supports all test cases
**Given** test configurations for tests 1, 2, 3, and 5  
**When** I run the workflow for each test  
**Then** each should generate appropriate visualizations  
**And** each should have test-specific analytical solutions  
**And** all should complete without errors (except test 3 which may have known instabilities)

---

## Feature: Output Quality and Correctness

**As a** scientist using SPH simulations  
**I want** output data and visualizations to be scientifically accurate  
**So that** I can trust the results for publication and analysis

### Scenario: Analytical solution comparison is accurate
**Given** a Sod shock tube simulation (test 1)  
**When** visualizations are generated  
**Then** the analytical solution should correctly show:
  - Shock discontinuity at the correct position  
  - Contact discontinuity at the correct position  
  - Rarefaction wave structure  
  - Correct density jump ratios  
  - Correct pressure values in all regions

### Scenario: CSV data integrity
**Given** a completed simulation  
**When** I inspect the CSV files  
**Then** each file should have a consistent header row  
**And** all columns should contain valid numerical data  
**And** no NaN or Inf values should be present (except in known vacuum regions)  
**And** time values should be monotonically increasing

### Scenario: Visualization plot limits are sensible
**Given** a visualization with extreme value ranges (e.g., vacuum test)  
**When** the plot is generated  
**Then** axis limits should be automatically adjusted to show relevant data  
**And** outliers should not cause entire plot to be unreadable  
**And** logarithmic scales should be used where appropriate (e.g., for density in vacuum)

---

## Implementation Checklist

### Code Changes Made:
- [x] Modified `src/core/output_format.cpp` to write metadata only once
- [x] Created `scripts/generate_visualizations.py` for automatic visualization
- [x] Created `run_with_visualization.sh` wrapper script
- [x] Fixed imports in visualization scripts

### Test Implementation Needed:
- [ ] Create `test/features/metadata_output.feature` (Gherkin)
- [ ] Create `test/features/visualization_generation.feature` (Gherkin)
- [ ] Implement step definitions in `test/features/steps/`
- [ ] Add pytest-bdd test runner configuration
- [ ] Create fixture for clean test environments
- [ ] Add CI/CD integration to run BDD tests

### Documentation Updates:
- [ ] Update workflow README with visualization instructions
- [ ] Document metadata.json format and location
- [ ] Add troubleshooting guide for visualization failures
- [ ] Document unit system handling

---

## Test Execution Examples

### Running BDD Tests (once implemented):
```bash
# Install pytest-bdd
pip install pytest-bdd

# Run all BDD tests
pytest test/features/

# Run specific feature
pytest test/features/metadata_output.feature

# Run with verbose output
pytest test/features/ -v --gherkin-terminal-reporter
```

### Manual Verification:
```bash
# Test metadata output
cd /Users/guo/OSS/sphcode/simulations/workflows/riemann_problems_workflow/01_simulation
export TEST_CASE=1
/Users/guo/OSS/sphcode/build/sph1d build/libriemann_plugin.dylib configs/test1_sod.json
ls results_test1_sod/riemann_problems/latest/outputs/csv/*.json
# Should show only: metadata.json

# Test visualization generation
./run_with_visualization.sh 1 configs/test1_sod.json
ls results_test1_sod/riemann_problems/latest/visualizations/
# Should show: initial_conditions.png, final_state.png, screenshots, animation
```

---

## Regression Prevention

### Protected Behaviors:
1. **Metadata Single File**: Automated test checks for exactly 1 metadata.json file
2. **No Per-Frame Metadata**: Test fails if any `0000*.meta.json` files exist
3. **Visualization Completeness**: Test verifies all required plots are generated
4. **Physical Units**: Test validates unit labels are present and correct
5. **Analytical Solutions**: Test compares SPH vs analytical within tolerance

### CI/CD Integration:
```yaml
# .github/workflows/bdd-tests.yml
name: BDD Tests
on: [push, pull_request]
jobs:
  test:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v2
      - name: Run BDD Tests
        run: |
          pytest test/features/ --gherkin-terminal-reporter
      - name: Upload test reports
        uses: actions/upload-artifact@v2
        with:
          name: bdd-test-results
          path: test/reports/
```
