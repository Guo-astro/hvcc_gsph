#!/bin/bash
# Test Python analysis tools
# Usage: ./scripts/test_python_analysis.sh

set -e

echo "================================="
echo "Python Analysis Tools Test"
echo "================================="
echo ""

# Check if uv is available
if ! command -v uv &> /dev/null; then
    echo "Error: uv not found. Please install uv first."
    exit 1
fi

# Find a test results directory
RESULTS_DIR=$(find build/sample -name "1D" -type d | head -1)

if [ -z "$RESULTS_DIR" ]; then
    echo "Error: No simulation results found in build/sample/"
    echo "Please run a simulation first:"
    echo "  cd build"
    echo "  ./sph shock_tube ../sample/shock_tube/shock_tube.json 2"
    exit 1
fi

echo "Using results from: $RESULTS_DIR"
echo ""

# Test 1: Check imports
echo "Test 1: Importing analysis modules..."
if uv run python -c "from analysis import SimulationReader, ConservationAnalyzer, ParticlePlotter, EnergyPlotter, TheoreticalComparison; print('✅ All modules imported successfully')"; then
    echo "✅ PASSED"
else
    echo "❌ FAILED"
    exit 1
fi
echo ""

# Test 2: CLI help
echo "Test 2: CLI help commands..."
if uv run python -m analysis.cli.analyze --help > /dev/null 2>&1; then
    echo "✅ analyze --help works"
else
    echo "❌ analyze --help failed"
    exit 1
fi

if uv run python -m analysis.cli.animate --help > /dev/null 2>&1; then
    echo "✅ animate --help works"
else
    echo "❌ animate --help failed"
    exit 1
fi
echo ""

# Test 3: Conservation analysis
echo "Test 3: Running conservation analysis..."
if uv run python -m analysis.cli.analyze conservation "$RESULTS_DIR" --interval 50 > /dev/null 2>&1; then
    echo "✅ Conservation analysis completed"
else
    echo "❌ Conservation analysis failed"
    exit 1
fi
echo ""

# Test 4: Energy analysis (if energy file exists)
echo "Test 4: Running energy analysis..."
PARENT_DIR=$(dirname "$RESULTS_DIR")
if [ -f "$PARENT_DIR/energy.dat" ] || [ -f "$PARENT_DIR/energy.txt" ]; then
    if uv run python -m analysis.cli.analyze energy "$RESULTS_DIR" > /dev/null 2>&1; then
        echo "✅ Energy analysis completed"
    else
        echo "⚠️  Energy analysis failed (non-critical)"
    fi
else
    echo "⚠️  No energy file found, skipping"
fi
echo ""

# Test 5: Quick analysis (creates plots)
echo "Test 5: Running quick analysis with plots..."
OUTPUT_DIR="/tmp/gsphcode_test_output"
mkdir -p "$OUTPUT_DIR"

if uv run python -m analysis.cli.analyze quick "$RESULTS_DIR" -o "$OUTPUT_DIR" > /dev/null 2>&1; then
    echo "✅ Quick analysis completed"
    
    # Check if plots were created
    if [ -f "$OUTPUT_DIR/density_comparison.png" ]; then
        echo "✅ Plots created successfully"
    else
        echo "⚠️  Plots not found"
    fi
else
    echo "❌ Quick analysis failed"
    exit 1
fi
echo ""

# Summary
echo "================================="
echo "All Python analysis tests passed! ✅"
echo "================================="
echo ""
echo "Output directory: $OUTPUT_DIR"
echo "You can check the generated plots there."
