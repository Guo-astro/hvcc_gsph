# GSPHCODE - Documentation Index

Complete documentation for the GSPH Smoothed Particle Hydrodynamics simulation framework.

## Quick Navigation

### 🚀 Getting Started
- **New to GSPH?** Start with [README.md](README.md) for project overview
- **Want to build?** See [Quick Start](#quick-start) below
- **Need examples?** Check [sample/](sample/) directories

### 📚 Documentation Files

| Document | Purpose | When to Read |
|----------|---------|--------------|
| **[README.md](README.md)** | Project overview, physics background, references | First time, understanding SPH methods |
| **[DEVELOPER_GUIDE.md](DEVELOPER_GUIDE.md)** | Complete development guide with examples | Adding new features, understanding architecture |
| **[QUICK_REFERENCE.md](QUICK_REFERENCE.md)** | Quick recipes for common tasks | Need a fast answer while coding |
| **[ARCHITECTURE.md](ARCHITECTURE.md)** | Visual diagrams and system design | Understanding code organization |
| **This file** | Navigation hub | Finding the right documentation |

### 🔍 What Do You Want to Do?

#### I want to run a simulation
1. Build the project (see [Quick Start](#quick-start))
2. Choose a sample from [sample/](sample/) or [production_sims/](production_sims/)
3. Run: `./build/sph <sample_name> <config.json> <threads>`

**Example:**
```bash
cd build && make
./sph shock_tube ../sample/shock_tube/shock_tube.json 8
```

#### I want to add a new simulation
→ **[QUICK_REFERENCE.md](QUICK_REFERENCE.md)** - "Add Sample Simulation" (5 min guide)  
→ **[DEVELOPER_GUIDE.md](DEVELOPER_GUIDE.md)** - "Adding New Simulations" (detailed)

#### I want to add a new algorithm (SPH variant)
→ **[QUICK_REFERENCE.md](QUICK_REFERENCE.md)** - "Add New SPH Algorithm"  
→ **[DEVELOPER_GUIDE.md](DEVELOPER_GUIDE.md)** - "Adding New Algorithms"

#### I want to understand the architecture
→ **[ARCHITECTURE.md](ARCHITECTURE.md)** - Visual diagrams and data flow  
→ **[DEVELOPER_GUIDE.md](DEVELOPER_GUIDE.md)** - "Architecture Overview"

#### I want to modify parameters
→ **[DEVELOPER_GUIDE.md](DEVELOPER_GUIDE.md)** - "Configuration Guide"  
→ Look at existing `.json` files in [sample/](sample/)

#### I'm getting build errors
→ **[DEVELOPER_GUIDE.md](DEVELOPER_GUIDE.md)** - "Debugging and Testing" section  
→ **[QUICK_REFERENCE.md](QUICK_REFERENCE.md)** - "Getting Unstuck"

#### I want to understand the code style
→ **.serena/memories/code_style_conventions.md** (if using Serena)  
→ **[DEVELOPER_GUIDE.md](DEVELOPER_GUIDE.md)** - "Best Practices"

#### I want to analyze simulation results
→ **[analysis/README.md](analysis/README.md)** - Analysis toolkit documentation  
→ Quick analysis: `python analysis/quick_analysis.py results/<sim_name>`

---

## Quick Start

### Prerequisites (macOS)
```bash
brew install cmake boost llvm libomp

# Add to ~/.zshrc:
export OpenMP_ROOT=$(brew --prefix)/opt/libomp
export PATH="/opt/homebrew/opt/llvm/bin:$PATH"
```

### Build
```bash
# Option 1: CMake (recommended)
rm -rf build && mkdir build
cmake -B build \
  -DOpenMP_C_FLAGS="-Xpreprocessor -fopenmp /opt/homebrew/opt/libomp/lib/libomp.dylib -I/opt/homebrew/opt/libomp/include" \
  -DOpenMP_CXX_FLAGS="-Xpreprocessor -fopenmp /opt/homebrew/opt/libomp/lib/libomp.dylib -I/opt/homebrew/opt/libomp/include" \
  -DOpenMP_C_LIB_NAMES=libomp \
  -DOpenMP_CXX_LIB_NAMES=libomp \
  -DOpenMP_libomp_LIBRARY="/opt/homebrew/opt/libomp/lib/libomp.dylib"
cd build && make

# Option 2: Makefile
make build

# Option 3: Nix Flakes
nix develop
mkdir -p build && cd build && cmake .. && make
```

### Run
```bash
./build/sph shock_tube ./sample/shock_tube/shock_tube.json 8
```

Results appear in `results/` directory.

---

## Project Structure

```
sphcode/
├── 📄 Documentation
│   ├── README.md              # Overview, physics, references
│   ├── DEVELOPER_GUIDE.md     # Complete dev guide
│   ├── QUICK_REFERENCE.md     # Quick recipes
│   ├── ARCHITECTURE.md        # System diagrams
│   └── DOCUMENTATION.md       # This file
│
├── 🔧 Build Configuration
│   ├── CMakeLists.txt         # CMake root
│   ├── Makefile               # Alternative build
│   └── flake.nix              # Nix development env
│
├── � Analysis Tools
│   ├── analysis/README.md     # Analysis toolkit guide
│   ├── analysis/requirements.txt  # Python dependencies
│   ├── analysis/*.py          # Analysis modules
│   └── analysis/*.py          # Command-line scripts
│
├── �📁 Source Code
│   ├── include/               # Headers
│   │   ├── simulation.hpp     # Core classes
│   │   ├── solver.hpp
│   │   ├── module*.hpp        # Module system
│   │   ├── gsph/              # GSPH variant
│   │   ├── disph/             # DISPH variant
│   │   └── kernel/            # Kernel functions
│   │
│   └── src/                   # Implementation
│       ├── main.cpp           # Entry point
│       ├── solver.cpp         # Main logic
│       ├── sample/            # Test simulations
│       ├── production_sims/   # Research sims
│       ├── gsph/              # GSPH modules
│       └── disph/             # DISPH modules
│
├── ⚙️ Configuration
│   ├── sample/                # Sample configs (.json)
│   └── production_sims/       # Production configs
│
├── 🧪 Testing
│   └── test/                  # Unit tests
│
└── 📊 Results
    └── results/               # Output (gitignored)
```

---

## Key Concepts

### SPH Variants
- **SSPH** (Standard SPH): Classic formulation, density-energy
- **DISPH** (Density Independent): Pressure-energy, better for contact discontinuities
- **GSPH** (Godunov SPH): Riemann solver based, automatic dissipation

### Module System
Physics is split into pluggable modules:
- **PreInteraction**: Neighbor finding, density calculation
- **FluidForce**: Pressure forces, artificial viscosity
- **GravityForce**: Self-gravity (Barnes-Hut tree)
- **Timestep**: Adaptive time step (CFL conditions)
- **HeatingCooling**: Energy sources/sinks

### Registration Pattern
Both samples and modules use compile-time registration:
```cpp
REGISTER_SAMPLE("name", function);     // Samples
REGISTER_MODULE("sph_type", "module_type", Class);  // Modules
```
No central registry to maintain - automatic!

### Configuration
JSON files control simulation parameters:
- Time stepping (start, end, output intervals)
- SPH method (type, kernel, neighbors)
- Physics (gamma, gravity)
- Numerical methods (CFL, artificial viscosity)

---

## Common Tasks Cheat Sheet

### Build
```bash
cd build && make                        # Build all
make clean                              # Clean build
rm -rf build && mkdir build            # Full rebuild needed if DIM changes
```

### Run
```bash
./build/sph <sample> <config.json> <threads>
make run_shock_tube                     # Via Makefile target
```

### Analyze Results
```bash
python analysis/quick_analysis.py results/<sim_name>
python analysis/shock_tube_analysis.py results/shock_tube 1.4
python analysis/make_animation.py results/<sim_name> -q dens -o output.mp4
```

### Add Sample
```cpp
// src/sample/my_sim.cpp
void load_my_sim(sim, param) { /* setup particles */ }
REGISTER_SAMPLE("my_sim", load_my_sim);
```

### Add Algorithm
```cpp
// src/gsph/g_my_module.cpp
class GMyModule : public Module { /* implement */ };
REGISTER_MODULE("gsph", "module_type", GMyModule);
```

### Change Dimension
```cpp
// include/defines.hpp
#define DIM 2  // 1, 2, or 3
// Then: rm -rf build && mkdir build && cd build && cmake .. && make
```

### Debug
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug
gdb --args ./build/sph shock_tube ...
```

---

## Learning Path

### Beginner
1. ✅ Read [README.md](README.md) overview
2. ✅ Build and run a sample simulation
3. ✅ Modify a JSON config and observe changes
4. ✅ Read [QUICK_REFERENCE.md](QUICK_REFERENCE.md) "Add Sample Simulation"
5. ✅ Create your first sample

### Intermediate
6. ✅ Read [ARCHITECTURE.md](ARCHITECTURE.md) to understand system design
7. ✅ Study existing module implementations in `src/gsph/`
8. ✅ Read [DEVELOPER_GUIDE.md](DEVELOPER_GUIDE.md) "Module System"
9. ✅ Create a simple module variant

### Advanced
10. ✅ Understand registration system internals
11. ✅ Implement a new SPH variant with full module set
12. ✅ Add new physics (MHD, radiation, chemistry)
13. ✅ Optimize performance (profiling, vectorization)

---

## Troubleshooting

| Problem | Solution | Reference |
|---------|----------|-----------|
| Sample not found | Check `REGISTER_SAMPLE` called, rebuild | QUICK_REFERENCE.md |
| Build errors | Install dependencies, check CMake flags | DEVELOPER_GUIDE.md "Quick Start" |
| Wrong dimension | Edit `defines.hpp`, full rebuild | DEVELOPER_GUIDE.md "Build System" |
| Slow performance | Enable OpenMP, use tree, profile | DEVELOPER_GUIDE.md "Best Practices" |
| Segfault | Check array bounds, use debugger | DEVELOPER_GUIDE.md "Debugging" |

---

## Contributing

When adding features:
1. ✅ Follow code style (see DEVELOPER_GUIDE.md "Best Practices")
2. ✅ Use registration macros (automatic integration)
3. ✅ Add JSON config for new samples
4. ✅ Test compilation and execution
5. ✅ Update documentation if adding major features

---

## Additional Resources

### In This Repository
- Sample configurations: [sample/](sample/)
- Production setups: [production_sims/](production_sims/)
- Kernel tests: [test/kernel_test/](test/kernel_test/)

### Serena MCP Memories (if using Serena)
- `.serena/memories/project_overview.md`
- `.serena/memories/architecture_deep_dive.md`
- `.serena/memories/refactoring_recommendations.md`
- `.serena/memories/suggested_commands.md`

### Physics References
See [README.md](README.md) for comprehensive bibliography on SPH methods.

---

## Version Information

- **C++ Standard**: C++14
- **CMake**: ≥ 3.23
- **Boost**: Required (filesystem, iostreams)
- **OpenMP**: Required for parallelization
- **Platform**: macOS (arm64), Linux (GCC 7.4.0+)

---

## Support

- **Code architecture**: Check [ARCHITECTURE.md](ARCHITECTURE.md)
- **How-to guides**: Check [DEVELOPER_GUIDE.md](DEVELOPER_GUIDE.md)
- **Quick answers**: Check [QUICK_REFERENCE.md](QUICK_REFERENCE.md)
- **Physics/math**: See references in [README.md](README.md)

---

**Happy simulating!** 🌌
