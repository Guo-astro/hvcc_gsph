#!/bin/bash
# build_all_dimensions.sh - Build executables for all dimensions
#
# Creates sph1d, sph2d, and sph3d in the build directory.
# Uses separate build directories (build_1d, build_2d, build_3d) to avoid
# conflicts, then copies executables to the main build directory.
#
# Usage:
#   ./scripts/build_all_dimensions.sh [build_dir]
#
# Arguments:
#   build_dir: Optional main build directory (default: build)

set -e  # Exit on error

# Configuration
FINAL_BUILD_DIR="${1:-build}"
PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

echo "=================================="
echo "Building GSPHCODE for all dimensions"
echo "=================================="
echo "Project root: $PROJECT_ROOT"
echo "Final build directory: $FINAL_BUILD_DIR"
echo ""

# Create final build directory if it doesn't exist
mkdir -p "$FINAL_BUILD_DIR"

# Build for each dimension
for DIM in 1 2 3; do
    echo "========================================"
    echo "Building ${DIM}D version (sph${DIM}d)..."
    echo "========================================"
    
    cd "$PROJECT_ROOT"
    
    # Use separate build directory for each dimension
    TEMP_BUILD_DIR="build_${DIM}d_temp"
    
    # Configure with specific dimension
    cmake -B "$TEMP_BUILD_DIR" -DBUILD_DIM=$DIM
    
    # Build library and executable
    # Each dimension gets its own build directory to avoid conflicts
    cmake --build "$TEMP_BUILD_DIR" --target sph${DIM}d -j$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)
    
    # Copy executable to final location
    cp "$TEMP_BUILD_DIR/sph${DIM}d" "$FINAL_BUILD_DIR/"
    
    # Verify executable was created
    if [ ! -f "$FINAL_BUILD_DIR/sph${DIM}d" ]; then
        echo "ERROR: Failed to build sph${DIM}d"
        exit 1
    fi
    
    echo "✓ sph${DIM}d built successfully"
    echo ""
done

# Clean up temporary build directories
echo "Cleaning up temporary build directories..."
rm -rf "$PROJECT_ROOT/build_1d_temp" "$PROJECT_ROOT/build_2d_temp" "$PROJECT_ROOT/build_3d_temp"

echo "=================================="
echo "Build Summary"
echo "=================================="
cd "$PROJECT_ROOT/$FINAL_BUILD_DIR"
ls -lh sph1d sph2d sph3d 2>/dev/null || true

echo ""
echo "All dimensions built successfully!"
echo "Executables:"
echo "  - $FINAL_BUILD_DIR/sph1d (1D simulations)"
echo "  - $FINAL_BUILD_DIR/sph2d (2D simulations)"
echo "  - $FINAL_BUILD_DIR/sph3d (3D simulations)"
echo ""
echo "Usage examples:"
echo "  ./$FINAL_BUILD_DIR/sph1d shock_tube"
echo "  ./$FINAL_BUILD_DIR/sph2d khi"
echo "  ./$FINAL_BUILD_DIR/sph3d evrard"
