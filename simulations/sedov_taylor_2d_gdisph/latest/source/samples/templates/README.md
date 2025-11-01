# Sample Templates

Reusable C++ templates for common simulation patterns. Templates reduce boilerplate and ensure consistent implementation across similar simulations.

## Purpose

Templates provide:
- **Code reuse**: Common initialization logic in one place
- **Consistency**: All shock tubes follow same pattern
- **Easy variants**: Create new tests by changing a few parameters
- **Type safety**: Compile-time checks for required methods

## Available Templates

### Shock Tube Template ✅ IMPLEMENTED

**File**: `shock_tube_template.hpp`

**Usage**:
```cpp
#include "samples/templates/shock_tube_template.hpp"
#include "core/sample_registry.hpp"

class SodShockTube : public ShockTubeTemplate<SodShockTube> {
protected:
    void set_left_state(real& dens, real& pres, real& vel) override {
        dens = 1.0;   // Left density
        pres = 1.0;   // Left pressure
        vel = 0.0;    // Left velocity
    }
    
    void set_right_state(real& dens, real& pres, real& vel) override {
        dens = 0.125; // Right density
        pres = 0.1;   // Right pressure
        vel = 0.0;    // Right velocity
    }
};

REGISTER_SAMPLE("sod_shock_tube", SodShockTube);
```

**Customization**:
- Override `compute_particle_mass()` for different mass distribution
- Override `compute_smoothing_length()` to adjust smoothing
- Modify member variables: `n_particles_`, `gamma_`, `x_min_`, `x_max_`, `x_discontinuity_`

**Benefits**:
- Automatic particle placement with correct density distribution
- Boundary conditions set correctly
- No duplicate code across shock tube variants
- Equal mass particles by default

### Disk Template ✅ IMPLEMENTED

**File**: `disk_template.hpp`

**Usage**:
```cpp
#include "samples/templates/disk_template.hpp"
#include "core/sample_registry.hpp"

class KeplerianDisk : public DiskTemplate<KeplerianDisk> {
protected:
    real surface_density(real r) override {
        // Power-law surface density profile
        const real sigma0 = 1.0;
        const real r0 = 1.0;
        return sigma0 * std::pow(r / r0, -1.5);
    }
    
    real rotation_velocity(real r) override {
        // Keplerian rotation curve
        const real G = 1.0;
        const real M_central = 1.0;
        return std::sqrt(G * M_central / r);
    }
};

REGISTER_SAMPLE("keplerian_disk", KeplerianDisk);
```

**Customization**:
- Override `vertical_density()` for 3D vertical structure
- Override `radial_velocity()` for inflow/outflow
- Override `vertical_velocity()` for vertical motion
- Override `compute_pressure()` for non-isothermal EOS
- Modify: `r_inner_`, `r_outer_`, `aspect_ratio_`, `n_particles_`, `temperature_`, `use_3d_`

**Benefits**:
- Automatic particle placement in cylindrical coordinates
- Rotation curve applied automatically
- Support for both 2D (razor-thin) and 3D (with vertical structure)
- Random particle distribution following density profile

## Template Design Pattern: CRTP

We use the **Curiously Recurring Template Pattern** (CRTP):

```cpp
template<typename Derived>
class TemplateBase {
public:
    void common_method() {
        // Call derived class methods
        auto params = static_cast<Derived*>(this)->get_params();
        // Use params...
    }
};

class Specific : public TemplateBase<Specific> {
public:
    auto get_params() { return /*...*/; }
};
```

**Advantages**:
- No virtual function overhead
- Static polymorphism (compile-time)
- Type-safe interface

## Implementation Guidelines

### For Template Authors

When creating a new template:

1. **Identify Common Pattern**
   - Look at 3+ similar simulations
   - Extract common initialization code
   - Identify what varies between them

2. **Design Interface**
   - Required methods (pure virtual or CRTP)
   - Optional customization points
   - Sensible defaults

3. **Document Interface**
   - What each method does
   - Expected return types
   - Valid parameter ranges

4. **Test with Examples**
   - Implement 2-3 examples using template
   - Ensure they match hand-written versions
   - Verify performance is equivalent

### For Template Users

When using a template:

1. **Inherit from Template**
   ```cpp
   class MySim : public MyTemplate<MySim>
   ```

2. **Implement Required Methods**
   - Check template documentation
   - Provide physics-specific parameters
   - Use type-safe return values

3. **Optional: Override Defaults**
   - Only override what's necessary
   - Call base class method if needed

4. **Test Your Instance**
   - Verify particle placement
   - Check initial conditions
   - Compare to expected solution

## Example: Shock Tube Template Implementation

**File**: `shock_tube_template.hpp`

```cpp
#pragma once
#include "core/simulation.hpp"
#include "core/parameters.hpp"
#include <tuple>

template<typename Derived>
class ShockTubeTemplate {
protected:
    // Required: derived class must implement these
    virtual std::tuple<real, real, real> set_left_state() const = 0;
    virtual std::tuple<real, real, real> set_right_state() const = 0;
    
    // Optional: override to customize
    virtual real get_discontinuity_position() const {
        return 0.5;  // Default: middle
    }
    
    virtual int get_num_particles() const {
        return 100;  // Default: 100 particles
    }

public:
    void initialize(Simulation* sim, SPHParameters* param) {
        auto [rho_L, p_L, v_L] = static_cast<const Derived*>(this)->set_left_state();
        auto [rho_R, p_R, v_R] = static_cast<const Derived*>(this)->set_right_state();
        
        real x_disc = get_discontinuity_position();
        int N = get_num_particles();
        
        real xmin = param->rangeMin[0];
        real xmax = param->rangeMax[0];
        real dx = (xmax - xmin) / N;
        
        for (int i = 0; i < N; ++i) {
            real x = xmin + (i + 0.5) * dx;
            real rho, p, v;
            
            if (x < x_disc) {
                rho = rho_L; p = p_L; v = v_L;
            } else {
                rho = rho_R; p = p_R; v = v_R;
            }
            
            SPHParticle particle;
            particle.pos[0] = x;
            particle.vel[0] = v;
            particle.dens = rho;
            particle.pres = p;
            particle.mass = rho * dx;  // Assumes unit cross-section
            particle.u = p / ((param->gamma - 1.0) * rho);
            
            sim->m_particles.push_back(particle);
        }
        
        sim->N = N;
    }
};
```

## Creating a New Template

### Step 1: Analyze Existing Code

Look at related simulations:
```bash
# Find all disk simulations
find src/samples/disks -name "*.cpp"

# Compare their initialization code
diff src/samples/disks/thin_disk_3d/thin_disk_3d.cpp \
     src/samples/disks/thin_slice/thin_slice_poly_2_5d.cpp
```

### Step 2: Extract Common Code

Create template header:
```bash
# Create new template
touch src/samples/templates/my_template.hpp

# Edit and add to version control
git add src/samples/templates/my_template.hpp
```

### Step 3: Implement Template

See example above for structure.

### Step 4: Create Example

```cpp
// src/samples/benchmarks/.../example_from_template.cpp
#include "samples/templates/my_template.hpp"

class Example : public MyTemplate<Example> {
    // Implement required methods
};

REGISTER_SAMPLE("example", [](auto* sim, auto* param) {
    Example ex;
    ex.initialize(sim, param);
});
```

### Step 5: Test

```bash
# Build
cd build && cmake .. && make

# Run
./sph example configs/.../example.json 8

# Compare to hand-written version
diff results/example/output_0008.txt \
     results/old_example/output_0008.txt
```

## Future Templates

Planned templates:

- [ ] **Vortex Template**: Gresho vortex, Taylor-Green, etc.
- [ ] **Collapse Template**: Evrard, Bonnor-Ebert sphere
- [ ] **Binary Template**: Binary stars, planet-disk
- [ ] **Stratified Template**: Layered fluids, atmospheres

## Contributing

To contribute a new template:

1. Identify pattern (3+ similar simulations)
2. Design interface (required/optional methods)
3. Implement template in this directory
4. Create 2-3 examples using it
5. Document usage in this README
6. Submit PR with tests

## Questions?

- **Template design**: See CRTP tutorials online
- **Implementation help**: Check existing templates
- **Physics questions**: Ask in discussion forum
- **Bugs**: Open GitHub issue

## References

- CRTP: https://en.wikipedia.org/wiki/Curiously_recurring_template_pattern
- C++ Templates: Vandevoorde & Josuttis, "C++ Templates: The Complete Guide"
