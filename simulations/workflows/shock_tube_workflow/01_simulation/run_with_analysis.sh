#!/bin/bash
#
# Run shock tube simulation with automatic post-processing
# 
# Usage: ./run_with_analysis.sh <config_file>
#

set -e

if [ $# -lt 1 ]; then
    echo "Usage: $0 <config_file>"
    echo "Example: $0 config/gdisph.json"
    exit 1
fi

CONFIG="$1"
PLUGIN="build/libshock_tube_plugin.dylib"
SPH1D="/Users/guo/OSS/sphcode/build/sph1d"

echo "========================================================================"
echo "SHOCK TUBE WORKFLOW WITH AUTOMATIC ANALYSIS"
echo "========================================================================"
echo "Config: $CONFIG"
echo ""

# Run simulation
echo "Step 1: Running simulation..."
$SPH1D $PLUGIN $CONFIG

# Find the latest results directory
OUTPUT_DIR=$(grep -o '"outputDirectory"[[:space:]]*:[[:space:]]*"[^"]*"' $CONFIG | sed 's/.*"\([^"]*\)"/\1/')
LATEST_DIR="${OUTPUT_DIR}/shock_tube/latest"

if [ ! -d "$LATEST_DIR" ]; then
    echo "ERROR: Results directory not found: $LATEST_DIR"
    exit 1
fi

echo ""
echo "Step 2: Generating visualizations..."
echo "Output directory: $LATEST_DIR"

# Run analysis script
PYTHONPATH=/Users/guo/OSS/sphcode/analysis:$PYTHONPATH \
python scripts/analyze_shock_tube.py "$LATEST_DIR/outputs/csv" 1.4

echo ""
echo "========================================================================"
echo "WORKFLOW COMPLETE"
echo "========================================================================"
echo "Results: $LATEST_DIR"
echo "Visualizations: $LATEST_DIR/visualizations"
echo ""
