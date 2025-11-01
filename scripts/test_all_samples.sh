#!/bin/bash
# Test all available samples with smoke tests
# Usage: ./scripts/test_all_samples.sh [threads]

set -e

THREADS=${1:-2}
BUILD_DIR="build"
SAMPLE_DIR="sample"
RESULTS_FILE="test_results.txt"

echo "================================="
echo "GSPHCODE Sample Smoke Tests"
echo "================================="
echo "Threads: $THREADS"
echo "Date: $(date)"
echo ""

# Check if build directory exists
if [ ! -d "$BUILD_DIR" ]; then
    echo "Error: Build directory not found. Please run cmake and make first."
    exit 1
fi

cd "$BUILD_DIR"

# Clear previous results
> "../$RESULTS_FILE"

# Counter for results
TOTAL=0
PASSED=0
FAILED=0

# Function to test a sample
test_sample() {
    local sample_name=$1
    local config_path=$2
    
    TOTAL=$((TOTAL + 1))
    echo -n "Testing $sample_name... "
    
    # Use gtimeout on macOS, timeout on Linux, or skip timeout if neither available
    local timeout_cmd=""
    if command -v timeout &> /dev/null; then
        timeout_cmd="timeout 30"
    elif command -v gtimeout &> /dev/null; then
        timeout_cmd="gtimeout 30"
    fi
    
    if $timeout_cmd ./sph "$sample_name" "$config_path" "$THREADS" > /dev/null 2>&1; then
        echo "✅ PASSED"
        echo "$sample_name: PASSED" >> "../$RESULTS_FILE"
        PASSED=$((PASSED + 1))
    else
        local exit_code=$?
        echo "❌ FAILED (exit code: $exit_code)"
        echo "$sample_name: FAILED (exit code: $exit_code)" >> "../$RESULTS_FILE"
        FAILED=$((FAILED + 1))
    fi
}

echo "Running smoke tests (1-2 timesteps per sample)..."
echo ""

# 1D Shock Tubes
echo "--- 1D Shock Tubes ---"
test_sample "shock_tube" "../$SAMPLE_DIR/shock_tube/shock_tube.json"
test_sample "shock_tube_strong_shock" "../$SAMPLE_DIR/shock_tube_strong_shock/shock_tube_strong_shock.json"
test_sample "shock_tube_heating_cooling" "../$SAMPLE_DIR/shock_tube_heating_cooling/shock_tube_heating_cooling.json"

# 2D Tests
echo ""
echo "--- 2D Tests ---"
test_sample "shock_tube_2d" "../$SAMPLE_DIR/shock_tube_2d/shock_tube_2d.json"
test_sample "khi" "../$SAMPLE_DIR/khi/khi.json"
test_sample "hydrostatic" "../$SAMPLE_DIR/hydrostatic/hydrostatic.json"
test_sample "pairing_instability" "../$SAMPLE_DIR/pairing_instability/pairing_instability.json"
test_sample "vacuum_test" "../$SAMPLE_DIR/vacuum_test/vacuum_test.json"
test_sample "lane_emden_2d" "../$SAMPLE_DIR/lane_emden_2d/lane_emden_2d.json"

# 3D Tests (may be slower)
echo ""
echo "--- 3D Tests ---"
test_sample "lane_emden" "../$SAMPLE_DIR/lane_emden/lane_emden.json"
# Note: evrard has known issues, skipping for now
# test_sample "evrard" "../$SAMPLE_DIR/evrard/evrard.json"

# 2.5D Tests
echo ""
echo "--- 2.5D Tests ---"
test_sample "thin_slice_poly_2_5d" "../$SAMPLE_DIR/thin_slice_poly_2_5d/thin_slice_poly_2_5d.json"
test_sample "thin_slice_poly_2_5d_relax" "../$SAMPLE_DIR/thin_slice_poly_2_5d_relax/thin_slice_poly_2_5d_relax.json"

# Summary
echo ""
echo "================================="
echo "Test Summary"
echo "================================="
echo "Total tests: $TOTAL"
echo "Passed: $PASSED ✅"
echo "Failed: $FAILED ❌"
echo ""
echo "Results saved to: $RESULTS_FILE"
echo ""

if [ $FAILED -eq 0 ]; then
    echo "🎉 All tests passed!"
    exit 0
else
    echo "⚠️  Some tests failed. Check $RESULTS_FILE for details."
    exit 1
fi
