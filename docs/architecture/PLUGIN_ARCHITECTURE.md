# Plugin Architecture

**SPHCode Simulation Plugin System**  
**Status**: ✅ Production Ready  
**Last Updated**: 2025-11-01

---

## Table of Contents

1. [Overview](#overview)
2. [Architecture](#architecture)
3. [Plugin Interface](#plugin-interface)
4. [Building Plugins](#building-plugins)
5. [Running Plugins](#running-plugins)
6. [Creating New Simulations](#creating-new-simulations)
7. [Implementation Details](#implementation-details)
8. [Benefits & Best Practices](#benefits--best-practices)

---

## Overview

The SPHCode plugin system allows simulations to be self-contained, dynamically-loaded modules instead of code compiled into the main executable. This enables:

- **Single Source of Truth**: All simulation code lives in `simulations/`
- **Hot Reload**: Recompile plugins without rebuilding the main executable
- **Complete Reproducibility**: Each run saves the exact source code used
- **Easy Distribution**: Share entire simulation directories
- **Clean Separation**: Core library independent of user simulations

### Architecture Evolution

#### Old System (Pre-Plugin)
```
src/samples/                    # Compiled into executable
├── benchmarks/shock_tubes/sod_shock_tube.cpp
└── ...

simulations/                    # Just outputs
└── shock_tube/run_*/          # Results only
```

**Problems**:
- Source code in two locations
- Cannot remove `src/samples` (needed for compilation)
- Samples tightly coupled to main executable
- Recompiling samples requires rebuilding everything

#### New System (Plugin-Based)
```
simulations/                    # SINGLE SOURCE OF TRUTH
├── shock_tube/
│   ├── shock_tube.cpp         # Source HERE
│   ├── CMakeLists.txt         # Build config HERE
│   ├── build/libshock_tube_plugin.dylib  # Compiled plugin
│   └── run_*/                 # Results
│       ├── source/shock_tube.cpp  # Copy of exact source used
│       └── outputs/
├── sedov_taylor/
├── sedov_taylor_2d/
└── template/                  # Template for new simulations
```

**Benefits**:
- ✅ Single source of truth in `simulations/`
- ✅ `src/samples/` completely removed
- ✅ Each simulation self-contained and independent
- ✅ Hot reload capability
- ✅ Easy sharing and collaboration

---

## Architecture

### Core Components

```
┌─────────────────────────────────────────┐
│          Main Executable                │
│         (sph1d/sph2d/sph3d)            │
└───────────────┬─────────────────────────┘
                │
                ├─→ SimulationLoader (facade)
                │   ├─→ PluginLoader (new plugin system)
                │   └─→ SampleRegistry (backward compatibility)
                │
                ↓
    ┌───────────────────────────┐
    │   SimulationPlugin        │
    │   (interface)             │
    │   - get_name()            │
    │   - get_description()     │
    │   - get_version()         │
    │   - initialize()          │
    │   - get_source_files()    │
    └───────────────────────────┘
                ↑
                │ implements
                │
    ┌───────────────────────────┐
    │   Concrete Plugin         │
    │   (ShockTubePlugin, etc.) │
    │   - Loaded from .dylib    │
    └───────────────────────────┘
```

### File Structure

Each simulation directory is self-contained:

```
simulations/{simulation_name}/
├── {simulation_name}.cpp       # Source code
├── CMakeLists.txt              # Build configuration
├── build.sh                    # Convenience build script
├── config.json                 # Default configuration (optional)
├── README.md                   # Documentation
├── analytical/                 # Analytical solutions (optional)
├── build/                      # Build artifacts (gitignored)
│   └── lib{simulation_name}_plugin.{dylib|so}
├── latest -> run_YYYY-MM-DD... # Symlink to latest run
└── run_YYYY-MM-DD_HHMMSS.../   # Individual simulation runs
    ├── metadata.json
    ├── config.json
    ├── initial_conditions.csv
    ├── source/                 # COPY of exact code used
    │   └── {simulation_name}.cpp
    └── outputs/
        ├── csv/
        └── binary/
```

---

## Plugin Interface

### Base Class

All plugins implement the `SimulationPlugin` interface:

```cpp
// include/core/simulation_plugin.hpp
namespace sph {

class SimulationPlugin {
public:
    virtual ~SimulationPlugin() = default;
    
    // Identification
    virtual std::string get_name() const = 0;
    virtual std::string get_description() const = 0;
    virtual std::string get_version() const = 0;
    
    // Core functionality
    virtual void initialize(
        std::shared_ptr<Simulation> sim,
        std::shared_ptr<SPHParameters> params
    ) = 0;
    
    // Source tracking
    virtual std::vector<std::string> get_source_files() const = 0;
};

} // namespace sph
```

### Example Implementation

```cpp
// simulations/shock_tube/shock_tube.cpp
#include "core/simulation_plugin.hpp"
#include "core/simulation.hpp"

class ShockTubePlugin : public sph::SimulationPlugin {
public:
    std::string get_name() const override {
        return "shock_tube";
    }
    
    std::string get_description() const override {
        return "Sod shock tube problem - 1D Riemann problem";
    }
    
    std::string get_version() const override {
        return "2.0.0";
    }
    
    void initialize(
        std::shared_ptr<sph::Simulation> sim,
        std::shared_ptr<sph::SPHParameters> params
    ) override {
        // Set up particles, initial conditions, etc.
        int N = 500;
        real dx = params->domain_size / N;
        
        for (int i = 0; i < N; i++) {
            real x = i * dx;
            SPHParticle p;
            p.r[0] = x;
            
            if (x < params->domain_size / 2.0) {
                // Left state
                p.dens = 1.0;
                p.pres = 1.0;
            } else {
                // Right state
                p.dens = 0.125;
                p.pres = 0.1;
            }
            
            sim->add_particle(p);
        }
    }
    
    std::vector<std::string> get_source_files() const override {
        return {"shock_tube.cpp"};
    }
};

// Export plugin
DEFINE_SIMULATION_PLUGIN(ShockTubePlugin)
```

### Plugin Export Macro

The `DEFINE_SIMULATION_PLUGIN` macro automatically generates the C export functions:

```cpp
// Expands to:
extern "C" sph::SimulationPlugin* create_plugin() {
    return new ShockTubePlugin();
}

extern "C" void destroy_plugin(sph::SimulationPlugin* plugin) {
    delete plugin;
}
```

---

## Building Plugins

### Build Configuration

Each plugin has its own `CMakeLists.txt`:

```cmake
# simulations/shock_tube/CMakeLists.txt
cmake_minimum_required(VERSION 3.20)
project(shock_tube_plugin)

# Dimension (1, 2, or 3)
if(NOT DEFINED DIM)
    set(DIM 1)
endif()

# C++17 required
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find SPH library
set(CMAKE_PREFIX_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../build")
find_package(SPH REQUIRED)

# Create plugin library
add_library(shock_tube_plugin SHARED shock_tube.cpp)

target_link_libraries(shock_tube_plugin
    PRIVATE
        sph::core
        sph::algorithms
)

target_compile_definitions(shock_tube_plugin PRIVATE DIM=${DIM})

# Output to build directory
set_target_properties(shock_tube_plugin PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/build"
)
```

### Build Script

Convenience script for building:

```bash
#!/bin/bash
# simulations/shock_tube/build.sh

# Dimension (default: 1)
DIM=${1:-1}

# Create build directory
mkdir -p build
cd build

# Configure and build
cmake -DDIM=$DIM ..
cmake --build .

echo "✅ Plugin built: libshock_tube_plugin.dylib"
echo "Run with: cd ../../build && ./sph${DIM}d ../simulations/shock_tube/build/libshock_tube_plugin.dylib"
```

### Building

```bash
cd simulations/shock_tube

# Build for 1D
./build.sh 1

# Build for 2D  
./build.sh 2

# Or manually
mkdir build && cd build
cmake -DDIM=1 ..
make
```

---

## Running Plugins

### Method 1: Direct Path

```bash
cd build
./sph1d ../simulations/shock_tube/build/libshock_tube_plugin.dylib
```

### Method 2: Convenience Script (if available)

```bash
cd build
./run_simulation shock_tube
```

### Output

```
=== Loaded Plugin ===
Name: shock_tube
Description: Sod shock tube problem - 1D Riemann problem
Version: 2.0.0
=====================

Initializing Sod shock tube problem...
Created 500 particles
  Left state:  ρ=1, P=1
  Right state: ρ=0.125, P=0.1

Running simulation...
Output directory: simulations/shock_tube/run_2025-11-01_104825_SSPH_1d/
```

### Result Structure

```
simulations/shock_tube/
├── latest -> run_2025-11-01_104825_SSPH_1d/
└── run_2025-11-01_104825_SSPH_1d/
    ├── metadata.json              # Git hash, performance
    ├── config.json                # Exact config used
    ├── initial_conditions.csv     # ICs
    ├── source/shock_tube.cpp      # COPY of source used
    └── outputs/
        ├── csv/00000.csv
        └── binary/00000.sph
```

---

## Creating New Simulations

### Step 1: Copy Template

```bash
cp -r simulations/template simulations/my_simulation
cd simulations/my_simulation
```

### Step 2: Rename Files

```bash
mv template_simulation.cpp my_simulation.cpp
```

### Step 3: Edit Source Code

```cpp
// my_simulation.cpp
class MySimulationPlugin : public sph::SimulationPlugin {
public:
    std::string get_name() const override {
        return "my_simulation";
    }
    
    std::string get_description() const override {
        return "Description of my simulation";
    }
    
    std::string get_version() const override {
        return "1.0.0";
    }
    
    void initialize(
        std::shared_ptr<sph::Simulation> sim,
        std::shared_ptr<sph::SPHParameters> params
    ) override {
        // Your initialization code here
        // Set up particles, initial conditions, etc.
    }
    
    std::vector<std::string> get_source_files() const override {
        return {"my_simulation.cpp"};
    }
};

DEFINE_SIMULATION_PLUGIN(MySimulationPlugin)
```

### Step 4: Update CMakeLists.txt

```cmake
# Change project name
project(my_simulation_plugin)

# Change target name
add_library(my_simulation_plugin SHARED my_simulation.cpp)

# Update link libraries if needed
target_link_libraries(my_simulation_plugin PRIVATE sph::core)
```

### Step 5: Build and Run

```bash
./build.sh
cd ../../build
./sph1d ../simulations/my_simulation/build/libmy_simulation_plugin.dylib
```

---

## Implementation Details

### Dynamic Library Loading

The `PluginLoader` uses `dlopen()`/`dlsym()` (Unix/Linux) or `LoadLibrary`/`GetProcAddress` (Windows) to load plugins at runtime:

```cpp
// Simplified version
class PluginLoader {
    void* handle_;
    
public:
    std::unique_ptr<SimulationPlugin> load(const std::string& path) {
        // Load library
        handle_ = dlopen(path.c_str(), RTLD_LAZY);
        if (!handle_) {
            throw std::runtime_error("Cannot load plugin: " + std::string(dlerror()));
        }
        
        // Get create function
        auto create = (CreatePluginFunc)dlsym(handle_, "create_plugin");
        if (!create) {
            throw std::runtime_error("Cannot find create_plugin symbol");
        }
        
        // Create plugin instance
        return std::unique_ptr<SimulationPlugin>(create());
    }
    
    ~PluginLoader() {
        if (handle_) dlclose(handle_);
    }
};
```

### SimulationLoader (Facade)

Provides unified interface for both plugins and old registry system:

```cpp
class SimulationLoader {
public:
    void load_and_initialize(
        const std::string& path_or_name,
        std::shared_ptr<Simulation> sim,
        std::shared_ptr<SPHParameters> params
    ) {
        if (is_plugin_path(path_or_name)) {
            // Load as plugin
            auto plugin = plugin_loader_.load(path_or_name);
            plugin->initialize(sim, params);
        } else {
            // Load from registry (backward compatibility)
            auto func = SampleRegistry::get(path_or_name);
            func(sim.get(), params.get());
        }
    }
    
private:
    bool is_plugin_path(const std::string& str) {
        return str.find(".dylib") != std::string::npos ||
               str.find(".so") != std::string::npos;
    }
    
    PluginLoader plugin_loader_;
};
```

### Source Code Archiving

Each run saves the plugin's source files:

```cpp
void SimulationRun::save_source_files(const SimulationPlugin& plugin) {
    auto source_files = plugin.get_source_files();
    
    for (const auto& file : source_files) {
        fs::path src = simulations_dir / sim_name / file;
        fs::path dst = run_dir / "source" / file;
        
        fs::create_directories(dst.parent_path());
        fs::copy_file(src, dst);
    }
}
```

### Backward Compatibility

The system supports both:

1. **Plugin Mode** (new): Load `.dylib`/`.so` files
2. **Registry Mode** (old): Use `SampleRegistry` for gradual migration

```cpp
// Both work:
solver.load_simulation("../simulations/shock_tube/build/libshock_tube_plugin.dylib");
solver.load_simulation("shock_tube");  // Registry (if still using src/samples)
```

---

## Benefits & Best Practices

### Benefits

1. **Single Source of Truth**
   - All simulation code lives in `simulations/`
   - No duplication between source and output directories
   - `src/samples/` can be completely removed

2. **Self-Contained**
   - Each simulation is independent
   - Own build system, dependencies, documentation
   - Easy to share entire simulation directory

3. **Hot Reload**
   - Recompile plugin without rebuilding main executable
   - Fast iteration during development
   - Core library remains stable

4. **Complete Reproducibility**
   - Every run saves exact source code used
   - Git hash links to specific code version
   - Can rebuild identical results years later

5. **Clean Separation**
   - Core library has no sample dependencies
   - User simulations don't pollute main codebase
   - Clear interface between library and applications

6. **Easy Distribution**
   - Share plugin `.dylib`/`.so` file
   - Share entire simulation directory
   - Collaborators can run without recompiling main code

### Best Practices

#### 1. Naming Conventions

- Plugin class: `{Name}Plugin` (e.g., `ShockTubePlugin`)
- Source file: `{name}.cpp` (lowercase, underscore-separated)
- Library: `lib{name}_plugin.{dylib|so}`
- Directory: `simulations/{name}/`

#### 2. Version Control

```cpp
std::string get_version() const override {
    return "2.1.0";  // Use semantic versioning
}
```

Increment:
- **Major**: Breaking changes to initial conditions or physics
- **Minor**: New features, backward compatible
- **Patch**: Bug fixes

#### 3. Documentation

Each simulation should have:
- `README.md`: Physics, usage, validation
- Inline comments: Explain non-obvious choices
- References: Papers, equations, validation sources

#### 4. Configuration

Provide `config.json` with sensible defaults:
```json
{
  "sph_type": "DISPH",
  "neighborNumber": 32,
  "gamma": 1.4,
  "cflSound": 0.3,
  "outputInterval": 0.01
}
```

#### 5. Testing

- Test plugin builds successfully
- Verify output matches expected results
- Check conservation properties
- Compare with analytical solutions (if available)

#### 6. Source Files

List all source files for archiving:
```cpp
std::vector<std::string> get_source_files() const override {
    return {
        "my_simulation.cpp",
        "helper.cpp",          // If using helper files
        "CMakeLists.txt",       // Build configuration
        "config.json"           // Default config
    };
}
```

---

## Migration Guide

### From src/samples to Plugin

If you have existing code in `src/samples/`:

#### 1. Identify Sample

```cpp
// Old: src/samples/benchmarks/my_sample.cpp
void load_my_sample(Simulation* sim, SPHParameters* param) {
    // initialization code
}
REGISTER_SAMPLE("my_sample", load_my_sample);
```

#### 2. Create Plugin Directory

```bash
mkdir -p simulations/my_sample
cd simulations/my_sample
cp ../../simulations/template/CMakeLists.txt .
cp ../../simulations/template/build.sh .
```

#### 3. Convert to Plugin

```cpp
// New: simulations/my_sample/my_sample.cpp
#include "core/simulation_plugin.hpp"

class MySamplePlugin : public sph::SimulationPlugin {
public:
    std::string get_name() const override { return "my_sample"; }
    std::string get_description() const override { return "..."; }
    std::string get_version() const override { return "1.0.0"; }
    
    void initialize(
        std::shared_ptr<sph::Simulation> sim,
        std::shared_ptr<sph::SPHParameters> params
    ) override {
        // Same initialization code as before
        // Change raw pointers to shared_ptr access: sim->... params->...
    }
    
    std::vector<std::string> get_source_files() const override {
        return {"my_sample.cpp"};
    }
};

DEFINE_SIMULATION_PLUGIN(MySamplePlugin)
```

#### 4. Update CMakeLists.txt

Change target name to `my_sample_plugin`.

#### 5. Build and Test

```bash
./build.sh
cd ../../build
./sph1d ../simulations/my_sample/build/libmy_sample_plugin.dylib
```

#### 6. Verify Output

Compare outputs with old system to ensure correctness.

---

## Summary

The plugin architecture provides:

✅ **Self-contained simulations** in `simulations/`  
✅ **Hot reload** capability  
✅ **Complete reproducibility** with source archiving  
✅ **Clean separation** between library and applications  
✅ **Easy sharing** and collaboration  
✅ **Backward compatibility** during migration  

### Current Status

- ✅ Plugin system implemented and tested
- ✅ Multiple simulations migrated (shock_tube, sedov_taylor, etc.)
- ✅ Template available for new simulations
- ✅ `src/samples/` removed (archived)
- ✅ Production ready

### Related Documentation

- [Migration History](../developer/MIGRATION_HISTORY.md) - Timeline of migrations
- [Simulation Run Structure](./SIMULATION_RUN_STRUCTURE.md) - Output organization
- [Creating Simulations](../../simulations/README.md) - Simulation overview
- [Template Usage](../../simulations/template/README.md) - Template guide

---

**The plugin system is production-ready and recommended for all new simulations!**
