#!/bin/bash
# Build script for Disk Relaxation Plugin (Step 1 of Razor-Thin HVCC Workflow)

set -e  # Exit on error

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="$SCRIPT_DIR/build"
# Navigate to actual build directory
SPH_BUILD_DIR="$(cd "$SCRIPT_DIR/../../../../build" && pwd)"

echo "=== Building Disk Relaxation Plugin ==="
echo "Plugin directory: $SCRIPT_DIR"
echo "SPH build directory: $SPH_BUILD_DIR"
echo

# Check if main SPH library exists
if [ ! -f "$SPH_BUILD_DIR/libsph_lib.a" ]; then
    echo "ERROR: Main SPH library not found at $SPH_BUILD_DIR/libsph_lib.a"
    echo "Please build the main project first:"
    echo "  cd $SPH_BUILD_DIR"
    echo "  cmake .. -DDIM=3"
    echo "  make"
    exit 1
fi

# Create build directory
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# Configure with CMake
echo "Configuring with CMake..."
cmake .. -DDIM=3

# Build
echo "Building plugin..."
make

echo
echo "=== Build Complete ==="
echo "Plugin location: $BUILD_DIR/libdisk_relaxation_plugin.dylib"
echo
echo "To run the relaxation simulation:"
echo "  cd $SCRIPT_DIR"
echo "  $SPH_BUILD_DIR/sph3d build/libdisk_relaxation_plugin.dylib config.json"
