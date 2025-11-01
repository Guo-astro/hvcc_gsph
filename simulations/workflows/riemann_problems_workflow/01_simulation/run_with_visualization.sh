#!/bin/bash
#
# Run Riemann problem simulation with automatic visualization generation
#
# Usage: ./run_with_visualization.sh <test_case> <config_file>
# Example: ./run_with_visualization.sh 1 configs/test1_sod.json
#

set -e

if [ $# -lt 2 ]; then
    echo "Usage: $0 <test_case> <config_file>"
    echo ""
    echo "Examples:"
    echo "  $0 1 configs/test1_sod.json"
    echo "  $0 2 configs/test2_rarefaction.json"
    echo "  $0 3 configs/test3_strong.json"
    echo "  $0 5 configs/test5_vacuum.json"
    exit 1
fi

TEST_CASE=$1
CONFIG_FILE=$2
PLUGIN="build/libriemann_plugin.dylib"
SPH1D="/Users/guo/OSS/sphcode/build/sph1d"
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

# Determine test name from config file
TEST_NAME=$(basename "$CONFIG_FILE" .json)

echo "========================================================================"
echo "RIEMANN PROBLEM WORKFLOW WITH AUTOMATIC VISUALIZATION"
echo "========================================================================"
echo "Test Case: $TEST_CASE"
echo "Config: $CONFIG_FILE"
echo "Test Name: $TEST_NAME"
echo ""

# Run simulation
echo "Step 1: Running simulation..."
echo "------------------------------------------------------------------------"
export TEST_CASE
$SPH1D $PLUGIN $CONFIG_FILE

# Find the latest results directory
OUTPUT_DIR=$(grep -o '"outputDirectory"[[:space:]]*:[[:space:]]*"[^"]*"' $CONFIG_FILE | sed 's/.*"\([^"]*\)"/\1/')
LATEST_DIR="${OUTPUT_DIR}/riemann_problems/latest"

if [ ! -d "$LATEST_DIR" ]; then
    echo "ERROR: Results directory not found: $LATEST_DIR"
    exit 1
fi

CSV_DIR="$LATEST_DIR/outputs/csv"
VIZ_DIR="$LATEST_DIR/visualizations"

echo ""
echo "Step 2: Generating visualizations..."
echo "------------------------------------------------------------------------"
echo "CSV directory: $CSV_DIR"
echo "Visualization directory: $VIZ_DIR"
echo ""

# Run visualization script
PYTHONPATH=/Users/guo/OSS/sphcode/analysis:$PYTHONPATH \
python "$SCRIPT_DIR/scripts/generate_visualizations.py" \
    "$CSV_DIR" \
    "$VIZ_DIR" \
    "$TEST_NAME" \
    1.4

EXITCODE=$?

echo ""
echo "========================================================================"
if [ $EXITCODE -eq 0 ]; then
    echo "WORKFLOW COMPLETE - SUCCESS"
    echo "========================================================================"
    echo "Results: $LATEST_DIR"
    echo "CSV data: $CSV_DIR"
    echo "Visualizations: $VIZ_DIR"
    echo ""
    echo "Generated visualizations:"
    ls -1 "$VIZ_DIR"
else
    echo "WORKFLOW FAILED"
    echo "========================================================================"
    echo "Visualization generation failed with exit code: $EXITCODE"
    echo "Simulation results: $CSV_DIR"
fi
echo "========================================================================"
echo ""

exit $EXITCODE
