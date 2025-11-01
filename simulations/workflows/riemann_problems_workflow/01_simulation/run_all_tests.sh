#!/bin/bash
#
# Run all Riemann problem test cases
#

set -e

WORKFLOW_DIR="/Users/guo/OSS/sphcode/simulations/workflows/riemann_problems_workflow/01_simulation"
SPH_BIN="/Users/guo/OSS/sphcode/build/sph1d"
PLUGIN="$WORKFLOW_DIR/build/libriemann_plugin.dylib"

cd "$WORKFLOW_DIR"

echo "======================================================================="
echo "Running 1D Riemann Problems Benchmark Suite"
echo "======================================================================="

# Test 1: Sod shock tube
echo ""
echo "=== Test 1: Sod Shock Tube ==="
export TEST_CASE=1
"$SPH_BIN" "$PLUGIN" configs/test1_sod.json

# Test 2: Double rarefaction
echo ""
echo "=== Test 2: Double Rarefaction ==="
export TEST_CASE=2
"$SPH_BIN" "$PLUGIN" configs/test2_rarefaction.json

# Test 3: Strong shock
echo ""
echo "=== Test 3: Strong Shock ==="
export TEST_CASE=3
"$SPH_BIN" "$PLUGIN" configs/test3_strong.json

# Test 5: Vacuum generation
echo ""
echo "=== Test 5: Vacuum Generation ==="
export TEST_CASE=5
"$SPH_BIN" "$PLUGIN" configs/test5_vacuum.json

echo ""
echo "======================================================================="
echo "All tests complete!"
echo "======================================================================="
echo ""
echo "To analyze results, run:"
echo "  python scripts/analyze_all_tests.py"
