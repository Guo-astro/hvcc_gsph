#!/bin/bash
# smart_run.sh - Automatically run the correct dimension executable
#
# Reads the config file, detects the required dimension, and runs
# the appropriate sphXd executable.
#
# Usage:
#   ./scripts/smart_run.sh <config_file> [build_dir]
#
# Example:
#   ./scripts/smart_run.sh sample/khi/config.toml
#   ./scripts/smart_run.sh sample/shock_tube/config.toml build_manual

set -e

# Check arguments
if [ $# -lt 1 ]; then
    echo "Usage: $0 <config_file> [build_dir]"
    echo ""
    echo "Examples:"
    echo "  $0 sample/shock_tube/config.toml"
    echo "  $0 sample/khi/config.toml build_manual"
    exit 1
fi

CONFIG_FILE="$1"
BUILD_DIR="${2:-build}"
PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

# Validate config file exists
if [ ! -f "$CONFIG_FILE" ]; then
    echo "ERROR: Config file not found: $CONFIG_FILE"
    exit 1
fi

# Detect dimension from config file
# Look for rangeMin/rangeMax array size in domain section
if ! grep -q "rangeMin" "$CONFIG_FILE" || ! grep -q "rangeMax" "$CONFIG_FILE"; then
    echo "ERROR: Could not find rangeMin/rangeMax in config file"
    exit 1
fi

# Extract rangeMin array and count elements
RANGE_MIN=$(grep "rangeMin" "$CONFIG_FILE" | sed 's/.*rangeMin[[:space:]]*=[[:space:]]*\[//' | sed 's/\].*//')
DIM=$(echo "$RANGE_MIN" | awk -F',' '{print NF}')

if [ -z "$DIM" ] || [ "$DIM" -lt 1 ] || [ "$DIM" -gt 3 ]; then
    echo "ERROR: Could not detect valid dimension from config file"
    echo "Found rangeMin = [$RANGE_MIN]"
    exit 1
fi

echo "=================================="
echo "GSPHCODE Smart Runner"
echo "=================================="
echo "Config file: $CONFIG_FILE"
echo "Detected dimension: ${DIM}D"
echo "Using executable: sph${DIM}d"
echo "=================================="
echo ""

# Check if executable exists
EXECUTABLE="$PROJECT_ROOT/$BUILD_DIR/sph${DIM}d"
if [ ! -f "$EXECUTABLE" ]; then
    echo "ERROR: Executable not found: $EXECUTABLE"
    echo ""
    echo "Build the ${DIM}D version with:"
    echo "  cmake -B $BUILD_DIR -DBUILD_DIM=$DIM"
    echo "  cmake --build $BUILD_DIR"
    echo ""
    echo "Or build all dimensions with:"
    echo "  ./scripts/build_all_dimensions.sh"
    exit 1
fi

# Run the simulation
echo "Running simulation..."
"$EXECUTABLE" "$CONFIG_FILE"
