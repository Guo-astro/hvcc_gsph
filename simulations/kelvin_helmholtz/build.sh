#!/bin/bash
# Build script for Kelvin-Helmholtz simulation plugin

set -e  # Exit on error

cd "$(dirname "$0")"

echo "============================================="
echo "Building Kelvin-Helmholtz Simulation Plugin"
echo "============================================="

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
echo "============================================="
echo "Build complete!"
echo "============================================="
echo ""
echo "Plugin location:"
echo "  $(pwd)/kelvin_helmholtz_plugin.dylib"
echo ""
echo "To run:"
echo "  cd ../../build"
echo "  ./sph2d ../simulations/kelvin_helmholtz/build/kelvin_helmholtz_plugin.dylib"
echo ""
