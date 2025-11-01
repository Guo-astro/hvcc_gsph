# Template Simulation Case

This is a template for creating new self-contained simulation cases.

## Quick Start

### 1. Copy this template

```bash
cd simulations/
cp -r template my_new_simulation
cd my_new_simulation
```

### 2. Rename and edit the source file

```bash
mv template_simulation.cpp my_new_simulation.cpp
# Edit my_new_simulation.cpp
```

Update the class name and simulation logic:
- Change `TemplateSimulationPlugin` to `MyNewSimulationPlugin`
- Modify `get_name()` to return your simulation name
- Implement your initial conditions in `initialize()`

### 3. Update CMakeLists.txt

```cmake
# Change:
add_library(template_simulation_plugin SHARED template_simulation.cpp)
# To:
add_library(my_new_simulation_plugin SHARED my_new_simulation.cpp)
```

### 4. Build the plugin

```bash
mkdir build
cd build
cmake ..
make
```

This creates `my_new_simulation_plugin.dylib` (macOS) or `.so` (Linux).

### 5. Run your simulation

```bash
cd /path/to/sphcode/build
./sph1d ../simulations/my_new_simulation/build/my_new_simulation_plugin.dylib
```

Or use the convenience script:

```bash
./run_simulation my_new_simulation
```

## File Structure

```
my_new_simulation/
├── my_new_simulation.cpp    # Your simulation code
├── CMakeLists.txt            # Build configuration
├── config.json               # Default runtime config (optional)
├── README.md                 # Documentation
├── build/                    # Build directory (gitignored)
│   └── my_new_simulation_plugin.dylib
└── run_YYYY-MM-DD_*/        # Output from runs
    ├── my_new_simulation.cpp     # Copy of source
    ├── metadata.json
    └── outputs/
```

## Simulation Code Structure

```cpp
#include "core/simulation_plugin.hpp"

class MySimulationPlugin : public SimulationPlugin {
public:
    std::string get_name() const override {
        return "my_simulation";
    }
    
    std::string get_description() const override {
        return "Description of what this simulates";
    }
    
    void initialize(std::shared_ptr<Simulation> sim,
                   std::shared_ptr<SPHParameters> params) override {
        // Set up initial conditions
        // Add particles with sim->add_particle(...)
    }
    
    std::vector<std::string> get_source_files() const override {
        return {"my_simulation.cpp"};
    }
};

// Export the plugin
DEFINE_SIMULATION_PLUGIN(MySimulationPlugin)
```

## Tips

1. **Use descriptive names**: Choose a name that describes what you're simulating
2. **Document parameters**: Add comments explaining key parameters
3. **Version your simulations**: Update `get_version()` when you change the code
4. **Test incrementally**: Start simple, verify it works, then add complexity
5. **Archive important runs**: The system automatically saves source code with each run

## Examples

See these simulations for reference:
- `simulations/shock_tube/` - 1D shock tube problem
- `simulations/sedov_taylor/` - Sedov-Taylor blast wave
