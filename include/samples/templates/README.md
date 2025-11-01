# C++ Templates for SPH Simulations

This directory contains C++ template classes (CRTP pattern) to help quickly create new simulation plugins.

## Available Templates

### 1. Shock Tube Template (`shock_tube_template.hpp`)

Base class for 1D Riemann problems and shock tubes.

**Usage**:
```cpp
#include "samples/templates/shock_tube_template.hpp"

class MySodShockTube : public ShockTubeTemplate<MySodShockTube> {
protected:
    void set_left_state(real& dens, real& pres, real& vel) override {
        dens = 1.0;  pres = 1.0;  vel = 0.0;
    }
    
    void set_right_state(real& dens, real& pres, real& vel) override {
        dens = 0.125;  pres = 0.1;  vel = 0.0;
    }
};
```

**Customizable Parameters**:
- `x_min_`, `x_max_` - Domain boundaries
- `x_discontinuity_` - Position of initial discontinuity
- `n_particles_` - Number of particles
- `gamma_` - Adiabatic index

### 2. Disk Template (`disk_template.hpp`)

Base class for 2D/3D disk simulations with rotation.

**Usage**:
```cpp
#include "samples/templates/disk_template.hpp"

class MyKeplerianDisk : public DiskTemplate<MyKeplerianDisk> {
protected:
    real surface_density(real r) override {
        return sigma0_ * std::pow(r / r0_, -1.5);  // Power law
    }
    
    real rotation_velocity(real r) override {
        return std::sqrt(G_ * M_central_ / r);  // Keplerian
    }
};
```

**Customizable Parameters**:
- `r_inner_`, `r_outer_` - Disk radial extent
- `n_particles_` - Number of particles
- `use_3d_` - Enable 3D disk (with vertical structure)
- `temperature_` - Disk temperature/sound speed
- `total_mass_` - Disk mass

## Using Templates in Plugins

Templates are designed to work with the plugin system:

```cpp
// In your plugin file (e.g., simulations/my_shock_tube/my_shock_tube.cpp)
#include "core/simulation_plugin.hpp"
#include "samples/templates/shock_tube_template.hpp"

namespace sph {

class MyShockTubePlugin : public SimulationPlugin {
public:
    std::string get_name() const override { return "my_shock_tube"; }
    
    void initialize(std::shared_ptr<Simulation> sim,
                   std::shared_ptr<SPHParameters> params) override {
        // Use template to set up initial conditions
        MyShockTubeTemplate template_instance;
        template_instance.load(sim.get(), params.get());
    }
};

DEFINE_SIMULATION_PLUGIN(MyShockTubePlugin)
}
```

## Benefits of Templates

✅ **Less boilerplate**: Focus on physics, not setup code  
✅ **Consistent patterns**: All shock tubes work the same way  
✅ **Easy customization**: Override only what you need  
✅ **Type safety**: Compile-time checking via CRTP  

## Alternative: Write from Scratch

You don't have to use templates! See `simulations/template/` for a minimal plugin example that doesn't use templates.

Templates are useful when:
- Creating many similar simulations (e.g., different shock tube variants)
- You want proven, tested initialization patterns
- You prefer declarative over imperative setup

Write from scratch when:
- Your simulation is unique
- You need full control
- Templates feel constraining

## Migration Note

These templates were moved from `/src/samples/templates/` during the plugin migration (Nov 2025). They remain available for use in new plugins but are no longer part of the main build - they're header-only templates included by plugins as needed.
