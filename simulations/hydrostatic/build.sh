#!/bin/bash
# Build script for hydrostatic simulation plugin

set -e  # Exit on error

cd "$(dirname "$0")"

echo "======================================"
echo "Building Hydrostatic Simulation Plugin"
echo "======================================"

# Create build directory
mkdir -p build
cd build

# Configure with CMake
echo "Configuring..."
cmake -DDIM=2 ..

# Build
echo "Building..."
make

echo ""
echo "======================================"
echo "Build complete!"
echo "======================================"
echo ""
echo "Plugin location:"
echo "  $(pwd)/hydrostatic_plugin.dylib"
echo ""
echo "To run:"
echo "  cd ../../build"
echo "  ./sph2d ../simulations/hydrostatic/build/hydrostatic_plugin.dylib"
echo ""
