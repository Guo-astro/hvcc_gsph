#!/bin/bash
# Build script for razor_thin_hvcc_gdisph flyby simulation

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="$SCRIPT_DIR/build"

# Create build directory
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# Configure with CMake
cmake ..

# Build
make

echo "Build complete! Plugin: $BUILD_DIR/librazor_thin_hvcc_gdisph_plugin.dylib"
