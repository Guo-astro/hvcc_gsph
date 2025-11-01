#!/bin/bash
# Quick test of a few samples
# Usage: ./scripts/test_quick_samples.sh [threads]

set -e

THREADS=${1:-2}
BUILD_DIR="build"
SAMPLE_DIR="sample"

echo "================================="
echo "GSPHCODE Quick Sample Tests"
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
    
    if ./sph "$sample_name" "$config_path" "$THREADS" > /dev/null 2>&1; then
        echo "✅ PASSED"
        PASSED=$((PASSED + 1))
    else
        local exit_code=$?
        echo "❌ FAILED (exit code: $exit_code)"
        FAILED=$((FAILED + 1))
    fi
}

echo "Running quick tests (full simulations)..."
echo ""

# Test a few representative samples
test_sample "shock_tube" "../$SAMPLE_DIR/shock_tube/shock_tube.json"
test_sample "khi" "../$SAMPLE_DIR/khi/khi.json"
test_sample "hydrostatic" "../$SAMPLE_DIR/hydrostatic/hydrostatic.json"

echo ""
echo "================================="
echo "Test Summary"
echo "================================="
echo "Total tests: $TOTAL"
echo "Passed: $PASSED ✅"
echo "Failed: $FAILED ❌"

if [ $FAILED -gt 0 ]; then
    echo ""
    echo "⚠️  Some tests failed."
    exit 1
else
    echo ""
    echo "✅ All tests passed!"
    exit 0
fi
