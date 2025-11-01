# Quick Start Guide

Get started with GSPHCODE in 5 minutes!

## Prerequisites

- **C++ Compiler**: Clang 18+ or GCC 7+
- **CMake**: 3.23+
- **Boost**: 1.67+
- **OpenMP**: For parallel execution
- **Python**: 3.9+ (for analysis tools)
- **uv**: Modern Python package manager

## Installation

### Option 1: Using Nix (Recommended)

```bash
# Clone repository
git clone https://github.com/Guo-astro/hvcc_gsph.git
cd hvcc_gsph

# Enter development environment (installs everything automatically)
nix develop

# Build
mkdir build && cd build
cmake ..
make -j8

# Setup Python tools
uv sync
```

### Option 2: Manual Installation (macOS)

```bash
# Install dependencies
brew install llvm libomp boost cmake

# Setup environment
export OpenMP_ROOT=$(brew --prefix)/opt/libomp
export PATH="/opt/homebrew/opt/llvm/bin:$PATH"
export LDFLAGS="-L/opt/homebrew/opt/llvm/lib"
export CPPFLAGS="-I/opt/homebrew/opt/llvm/include"

# Build
mkdir build && cd build
cmake ..
make -j8

# Install Python tools
curl -LsSf https://astral.sh/uv/install.sh | sh
uv sync
```

### Option 3: Manual Installation (Linux)

```bash
# Install dependencies (Ubuntu/Debian)
sudo apt-get update
sudo apt-get install clang libomp-dev libboost-all-dev cmake

# Build
mkdir build && cd build
cmake ..
make -j$(nproc)

# Install Python tools
curl -LsSf https://astral.sh/uv/install.sh | sh
uv sync
```

## Your First Simulation

### 1. Run a Shock Tube Test

```bash
cd build
./sph shock_tube ../sample/shock_tube/shock_tube.json 4

# Output will be in:
# build/sample/shock_tube/results/DISPH/shock_tube/1D/
```

### 2. Analyze Results

```bash
# Quick analysis with plots
uv run python -m analysis.cli.analyze quick \
    build/sample/shock_tube/results/DISPH/shock_tube/1D/ \
    -o plots/

# Check conservation
uv run python -m analysis.cli.analyze conservation \
    build/sample/shock_tube/results/DISPH/shock_tube/1D/ \
    --interval 50

# Create animation (1D)
uv run python -m analysis.cli.animate \
    build/sample/shock_tube/results/DISPH/shock_tube/1D/ \
    -q dens -o shock_tube.mp4 --fps 30
```

### 3. View Results

Plots are saved as PNG files in the output directory. Energy conservation summary is printed to terminal.

## Try More Examples

### 2D Kelvin-Helmholtz Instability

```bash
cd build
./sph khi ../sample/khi/khi.json 8

# Analyze
uv run python -m analysis.cli.analyze quick \
    build/sample/khi/results/GSPH/khi/2D/
```

### 3D Evrard Collapse (with self-gravity)

```bash
cd build
./sph evrard ../sample/evrard/evrard.json 8

# Note: This simulation has known issues, use for testing only
```

## Understanding the Output

### Directory Structure
```
build/sample/<sample_name>/results/<SPH_TYPE>/<sample_name>/<DIM>/
├── 00000.csv          # Initial state
├── 00001.csv          # Snapshot 1
├── ...
├── 00682.csv          # Final state
└── ../energy.dat      # Energy history (if available)
```

### CSV File Format
Each CSV contains:
- `time` - Simulation time
- `pos_x, pos_y, pos_z` - Particle positions
- `vel_x, vel_y, vel_z` - Velocities
- `dens` - Density
- `pres` - Pressure
- `ene` - Specific internal energy
- `mass` - Particle mass
- `sml` - Smoothing length
- Additional fields (alpha, shockSensor, etc.)

## Configuration

### Basic JSON Configuration

```json
{
  "extends": "../../configs/base/disph_1d.json",
  "outputDirectory": "sample/my_test/results",
  "endTime": 1.0,
  "outputTime": 0.01,
  "neighborNumber": 32,
  "gamma": 1.4,
  "SPHType": "disph"
}
```

### Key Parameters

- `SPHType`: "ssph", "disph", "gsph", or "gdisph"
- `neighborNumber`: Typical values 32-128
- `gamma`: Adiabatic index (1.4 for ideal gas)
- `cflSound`: CFL number for timestep (0.3 default)
- `periodic`: true/false for periodic boundaries
- `useGravity`: Enable self-gravity

See `configs/base/README.md` for complete parameter list.

## Common Tasks

### Change Dimension

Edit `include/utilities/defines.hpp`:
```cpp
#define DIM 2  // Change to 1, 2, or 3
```

Then rebuild:
```bash
cd build
make clean
make -j8
```

### Use Different SPH Method

Edit your JSON config:
```json
{
  "SPHType": "gsph"  // Options: ssph, disph, gsph, gdisph
}
```

### Enable Self-Gravity

```json
{
  "useGravity": true,
  "G": 1.0,
  "theta": 0.5
}
```

### Adjust Output Frequency

```json
{
  "outputTime": 0.01,    // Output every 0.01 time units
  "energyTime": 0.001    // Energy log frequency
}
```

## Testing

### Run Automated Tests

```bash
# Test all samples (smoke tests)
./scripts/test_all_samples.sh 4

# Test Python analysis tools
./scripts/test_python_analysis.sh
```

### Verify Installation

```bash
# Check C++ build
cd build
./sph shock_tube ../sample/shock_tube/shock_tube.json 1

# Check Python tools
uv run python -c "from analysis import SimulationReader; print('✅ OK')"
```

## Troubleshooting

### Build Errors

**Problem**: `fatal error: omp.h: No such file or directory`
```bash
# macOS:
brew install libomp
export OpenMP_ROOT=$(brew --prefix)/opt/libomp

# Linux:
sudo apt-get install libomp-dev
```

**Problem**: CMake can't find Boost
```bash
# macOS:
brew install boost

# Linux:
sudo apt-get install libboost-all-dev
```

### Runtime Errors

**Problem**: Simulation crashes or gives NaN
- Reduce timestep: Set `cflSound: 0.1` in JSON
- Increase artificial viscosity: Set `avAlpha: 1.5`
- Check initial conditions are valid

**Problem**: Conservation errors too large
- Increase neighbor number: `neighborNumber: 64`
- Use different SPH variant: Try "gsph"
- Enable time-dependent AV: `useTimeDependentAV: true`

### Python Errors

**Problem**: `ModuleNotFoundError: No module named 'analysis'`
```bash
# Make sure you're in the project root
cd /path/to/sphcode

# Run with uv
uv run python -m analysis.cli.analyze ...
```

**Problem**: No energy file found
- Energy file might be named `energy.dat` instead of `energy.txt`
- Check parent directory of results
- Some samples don't output energy history

## Next Steps

1. **Read the Documentation**:
   - [DEVELOPER_GUIDE.md](DEVELOPER_GUIDE.md) - In-depth development guide
   - [ARCHITECTURE.md](ARCHITECTURE.md) - Code structure
   - [QUICK_REFERENCE.md](QUICK_REFERENCE.md) - Command reference

2. **Explore Samples**:
   - Browse `src/samples/` for category READMEs
   - Each category has example problems

3. **Create Your Own Simulation**:
   - See [CONTRIBUTING.md](CONTRIBUTING.md) for step-by-step guide
   - Use configuration inheritance to reduce boilerplate

4. **Join the Community**:
   - Report bugs via GitHub Issues
   - Share results and ask questions

## Useful Commands

```bash
# Build in parallel
make -j8

# Clean and rebuild
make clean && make -j8

# List available samples (if implemented)
./sph --list-samples

# Get sample info (if implemented)
./sph --info shock_tube

# Run with specific thread count
./sph shock_tube config.json 8

# Monitor energy in real-time
tail -f sample/shock_tube/results/energy.dat

# Quick Python analysis
uv run python -m analysis.cli.analyze quick <results_dir>

# Conservation check
uv run python -m analysis.cli.analyze conservation <results_dir>

# Create animation
uv run python -m analysis.cli.animate <results_dir> -q dens
```

## Performance Tips

1. **Use OpenMP**: Set threads to number of CPU cores
2. **Compile with optimizations**: CMake does this by default
3. **Reduce output frequency**: Larger `outputTime` = fewer files
4. **Use appropriate neighbor number**: 32-64 for most cases
5. **Profile your code**: Use `perf` or `Instruments` on macOS

## Getting Help

- **Documentation**: Start with [README.md](README.md)
- **Examples**: Check `src/samples/*/README.md`
- **Issues**: Search or create GitHub Issues
- **Discussions**: Use GitHub Discussions for questions

---

**Happy Simulating!** 🚀

For questions or feedback, see [CONTRIBUTING.md](CONTRIBUTING.md).
