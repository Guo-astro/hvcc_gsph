#!/bin/bash
# Build script for Razor-thin HVCC simulation plugin

set -e  # Exit on error

cd "$(dirname "$0")"

echo "================================================"
echo "Building Razor-thin HVCC Simulation Plugin"
echo "================================================"

# Create build directory
mkdir -p build
cd build

# Configure with CMake (requires DIM=3)
echo "Configuring..."
cmake -DDIM=3 ..

# Build
echo "Building..."
make

echo ""
echo "================================================"
echo "Build complete!"
echo "================================================"
echo ""
echo "Plugin location:"
echo "  $(pwd)/razor_thin_hvcc_plugin.dylib"
echo ""
echo "To run:"
echo "  cd ../../build"
echo "  ./sph3d ../simulations/razor_thin_hvcc/build/razor_thin_hvcc_plugin.dylib"
echo ""
echo "Note: This simulation requires an initial conditions file."
echo "Set 'initialConditionsFile' in config.json before running."
echo ""
