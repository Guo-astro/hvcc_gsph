# Build, Test, and Run Commands

## Building the Project

### Using CMake (Recommended for macOS)
```bash
# Clean previous builds
rm -rf build && mkdir build

# Configure with OpenMP
cmake -B build \
  -DOpenMP_C_FLAGS="-Xpreprocessor -fopenmp /opt/homebrew/opt/libomp/lib/libomp.dylib -I/opt/homebrew/opt/libomp/include" \
  -DOpenMP_CXX_FLAGS="-Xpreprocessor -fopenmp /opt/homebrew/opt/libomp/lib/libomp.dylib -I/opt/homebrew/opt/libomp/include" \
  -DOpenMP_C_LIB_NAMES=libomp \
  -DOpenMP_CXX_LIB_NAMES=libomp \
  -DOpenMP_libomp_LIBRARY="/opt/homebrew/opt/libomp/lib/libomp.dylib"

# Build
cd build && make
```

### Using Makefile (Alternative)
```bash
# Build project
make build

# Build and run default sample
make all
```

### Using Nix Flakes (New)
```bash
# Enter development environment
nix develop

# Build inside nix environment
mkdir -p build && cd build && cmake .. && make
```

## Running Simulations

### Command Format
```bash
./sph <sample_name> [json_config] [num_threads]
```

### Examples
```bash
# Run with default parameters
./build/sph shock_tube 8

# Run with JSON configuration
./build/sph shock_tube ./sample/shock_tube/shock_tube.json 8

# Run production simulation
./build/sph razor_thin_hvcc ./production_sims/razor_thin_hvcc/razor_thin_hvcc.json 8
```

### Using Makefile Targets
```bash
# Sample simulations
make run_shock_tube
make run_evrard
make run_khi
make run_lane_emden
make run_sedov_taylor

# Production simulations
make run_razor_thin_hvcc
make run_razor_thin_hvcc_debug
make run_razor_thin_sg_relaxation
```

## Testing
```bash
# Run kernel tests
./build/sph kernel_test

# Or using make
make run_kernel_test
```

## Cleaning
```bash
# CMake build
rm -rf build

# Makefile build
make clean  # removes build_manual/
```

## Environment Setup (macOS)

### Prerequisites Installation
```bash
# Install Homebrew dependencies
brew install cmake boost llvm libomp

# Set up environment variables (add to ~/.zshrc)
export OpenMP_ROOT=$(brew --prefix)/opt/libomp
export PATH="/opt/homebrew/opt/llvm/bin:$PATH"
export LDFLAGS="-L/opt/homebrew/opt/llvm/lib"
export CPPFLAGS="-I/opt/homebrew/opt/llvm/include"
```

## Output
Results are written to:
- `results/` - Default output directory
- Particle data snapshots at specified intervals
- Energy output for monitoring conservation
- Log files for simulation progress
