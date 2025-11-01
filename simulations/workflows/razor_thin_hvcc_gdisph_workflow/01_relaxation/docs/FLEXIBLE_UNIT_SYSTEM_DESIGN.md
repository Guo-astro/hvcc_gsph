# Flexible Unit System - Now Global!

**NOTE**: The flexible unit system has been promoted to a global utility!

## New Location

The implementation is now available globally at:
```
include/utilities/flexible_unit_params.hpp
```

## Documentation

Complete documentation: [include/utilities/FLEXIBLE_UNIT_SYSTEM_README.md](../../../../include/utilities/FLEXIBLE_UNIT_SYSTEM_README.md)

## Usage in This Workflow

```cpp
#include "utilities/flexible_unit_params.hpp"

// Create unit system for disk
sph::FlexibleUnitParams params(
    3.0,                              // 3 pc radius
    1000.0,                           // 1000 M☉
    UnitSystemType::GALACTIC_PC,      // Input units
    UnitSystemType::GALACTIC_PC       // Output units (same)
);

params.print();  // Show configuration

// Use in simulation
real R_code = params.code.length;  // = 1.0 (dimensionless)
```

## Benefits

✅ **All workflows** can now use the flexible unit system  
✅ **Global documentation** in include/utilities/  
✅ **Consistent API** across entire codebase  
✅ **Self-documenting** output with proper unit labels  

---

*This file kept for historical reference. See global docs for current usage.*


## Philosophy

**Complete User Freedom: Input Units ≠ Output Units**

```
User Input (Astronomical) → Code (Dimensionless) → User Output (Any units!)
     ↓                            ↓                         ↓
  [pc, M☉, Myr]              [R=1, M=1, G=1]        [AU, M_earth, yr] or [pc, M☉] or [SI]
```

## Key Features

1. **Input Flexibility**: User specifies parameters in convenient units (parsecs, solar masses, etc.)
2. **Simulation Stability**: Internal computation always uses dimensionless units (O(1) values)
3. **Output Flexibility**: CSV/output can be in DIFFERENT units than input
4. **No Hard-Coding**: All conversions automatic via `UnitFactory`
5. **Type Safety**: Clear separation between input, code, and output values

## Implementation

### Step 1: Enhanced Parameters Struct

```cpp
#ifndef DISK_INITIALIZATION_PARAMS_HPP
#define DISK_INITIALIZATION_PARAMS_HPP

#include "utilities/defines.hpp"
#include "utilities/unit_conversions.hpp"
#include <iostream>
#include <iomanip>

namespace sph {

/**
 * @brief Flexible disk initialization with user-defined input and output units
 * 
 * Design: Input (user units) → Code (dimensionless) → Output (user units, can differ!)
 */
struct DiskInitializationParams {
    
    // Layer 1: User Input (in chosen units)
    struct InputParams {
        real radius;              // Disk radius
        real half_thickness;      // Half-thickness
        real total_mass;          // Total mass
        real polytropic_index;    // n (dimensionless)
        int grid_nx, grid_ny, grid_nz;
    } input;
    
    // Layer 2: Code Units (dimensionless - used in simulation)
    struct CodeParams {
        real radius;              // = 1.0 (normalized)
        real half_thickness;      // = z/R (ratio)
        real total_mass;          // = 1.0 (normalized)
        real G;                   // = 1.0 (normalized)
        real polytropic_index;    // n (dimensionless)
    } code;
    
    // Layer 3: Unit Conversion System
    struct Units {
        // Characteristic scales (SI)
        real length_SI;           // R_disk in meters
        real mass_SI;             // M_disk in kg
        real time_SI;             // sqrt(R³/GM) in seconds
        real velocity_SI;         // R/T in m/s
        real density_SI;          // M/R³ in kg/m³
        
        // Input/Output unit systems (from UnitFactory)
        UnitSystem input;         // How user specifies input
        UnitSystem output;        // How user wants output
        std::string input_name;   // e.g., "Galactic (pc)"
        std::string output_name;  // e.g., "SI"
    } units;
    
    /**
     * @brief Constructor with flexible input/output units
     * 
     * @param R_in Disk radius in input units
     * @param z_in Half-thickness in input units
     * @param M_in Total mass in input units
     * @param n Polytropic index
     * @param nx, ny, nz Grid resolution
     * @param input_type Unit system for input (default: Galactic pc)
     * @param output_type Unit system for output (default: same as input)
     */
    DiskInitializationParams(
        real R_in, real z_in, real M_in, real n,
        int nx, int ny, int nz,
        UnitSystemType input_type = UnitSystemType::GALACTIC_PC,
        UnitSystemType output_type = UnitSystemType::GALACTIC_PC)
    {
        using namespace PhysicalConstants;
        
        // Store input
        input.radius = R_in;
        input.half_thickness = z_in;
        input.total_mass = M_in;
        input.polytropic_index = n;
        input.grid_nx = nx;
        input.grid_ny = ny;
        input.grid_nz = nz;
        
        // Create unit systems
        units.input = UnitFactory::create(input_type);
        units.output = UnitFactory::create(output_type);
        units.input_name = UnitFactory::getTypeName(input_type);
        units.output_name = UnitFactory::getTypeName(output_type);
        
        // Convert input → SI (characteristic scales)
        units.length_SI = R_in / units.input.length_factor;
        units.mass_SI = M_in / units.input.mass_factor;
        units.time_SI = std::sqrt(units.length_SI * units.length_SI * units.length_SI 
                                  / (G_SI * units.mass_SI));
        units.velocity_SI = units.length_SI / units.time_SI;
        units.density_SI = units.mass_SI / std::pow(units.length_SI, 3);
        
        // Set code units (dimensionless)
        code.radius = 1.0;
        code.half_thickness = z_in / R_in;  // Dimensionless ratio
        code.total_mass = 1.0;
        code.G = 1.0;
        code.polytropic_index = n;
    }
    
    // ═══════════════════════════════════════════════════════════
    // Conversion Functions
    // ═══════════════════════════════════════════════════════════
    
    // Input units → Code units (for initialization)
    real input_to_code_length(real L) const {
        return (L / units.input.length_factor) / units.length_SI;
    }
    
    real input_to_code_mass(real M) const {
        return (M / units.input.mass_factor) / units.mass_SI;
    }
    
    // Code units → Output units (for CSV/visualization)
    real code_to_output_length(real L_code) const {
        real L_SI = L_code * units.length_SI;
        return L_SI * units.output.length_factor;
    }
    
    real code_to_output_mass(real M_code) const {
        real M_SI = M_code * units.mass_SI;
        return M_SI * units.output.mass_factor;
    }
    
    real code_to_output_density(real rho_code) const {
        real rho_SI = rho_code * units.density_SI;
        return rho_SI * units.output.density_factor;
    }
    
    real code_to_output_velocity(real v_code) const {
        real v_SI = v_code * units.velocity_SI;
        return v_SI * units.output.length_factor / units.output.time_factor;
    }
    
    real code_to_output_time(real t_code) const {
        real t_SI = t_code * units.time_SI;
        return t_SI * units.output.time_factor;
    }
    
    // ═══════════════════════════════════════════════════════════
    // Display
    // ═══════════════════════════════════════════════════════════
    
    void print() const {
        std::cout << "\n";
        std::cout << "╔══════════════════════════════════════════════════════════════════╗\n";
        std::cout << "║       Disk Initialization: Flexible Unit System                  ║\n";
        std::cout << "╠══════════════════════════════════════════════════════════════════╣\n";
        std::cout << "║ INPUT (User-Specified):                                          ║\n";
        std::cout << "║   Unit system:     " << std::setw(44) << std::left << units.input_name << "║\n";
        std::cout << "║   Radius:          " << std::setw(10) << input.radius 
                  << " " << std::setw(33) << std::left << units.input.length_unit << "║\n";
        std::cout << "║   Half-thickness:  " << std::setw(10) << input.half_thickness 
                  << " " << std::setw(33) << std::left << units.input.length_unit << "║\n";
        std::cout << "║   Total mass:      " << std::setw(10) << input.total_mass 
                  << " " << std::setw(33) << std::left << units.input.mass_unit << "║\n";
        std::cout << "║   Polytropic idx:  " << std::setw(44) << input.polytropic_index << "║\n";
        std::cout << "║   Grid resolution: " << input.grid_nx << " × " << input.grid_ny 
                  << " × " << std::setw(36) << std::left << input.grid_nz << "║\n";
        std::cout << "╠══════════════════════════════════════════════════════════════════╣\n";
        std::cout << "║ SIMULATION (Code Units - Dimensionless):                        ║\n";
        std::cout << "║   Radius:          " << std::setw(44) << code.radius << "║\n";
        std::cout << "║   Half-thickness:  " << std::setw(44) << code.half_thickness << "║\n";
        std::cout << "║   Total mass:      " << std::setw(44) << code.total_mass << "║\n";
        std::cout << "║   G:               " << std::setw(44) << code.G << "║\n";
        std::cout << "╠══════════════════════════════════════════════════════════════════╣\n";
        std::cout << "║ OUTPUT (User-Specified - can differ from input!):                ║\n";
        std::cout << "║   Unit system:     " << std::setw(44) << std::left << units.output_name << "║\n";
        std::cout << "║   Length unit:     " << std::setw(44) << std::left << units.output.length_unit << "║\n";
        std::cout << "║   Mass unit:       " << std::setw(44) << std::left << units.output.mass_unit << "║\n";
        std::cout << "║   Density unit:    " << std::setw(44) << std::left << units.output.density_unit << "║\n";
        std::cout << "╠══════════════════════════════════════════════════════════════════╣\n";
        std::cout << "║ CHARACTERISTIC SCALES (Internal - SI):                           ║\n";
        std::cout << "║   Length:  " << std::scientific << std::setprecision(4) 
                  << std::setw(12) << units.length_SI << " m" << std::setw(33) << " " << "║\n";
        std::cout << "║   Mass:    " << std::setw(12) << units.mass_SI << " kg" << std::setw(32) << " " << "║\n";
        std::cout << "║   Time:    " << std::setw(12) << units.time_SI << " s" << std::setw(33) << " " << "║\n";
        std::cout << "║   Velocity:" << std::setw(12) << units.velocity_SI << " m/s" << std::setw(31) << " " << "║\n";
        std::cout << "║   Density: " << std::setw(12) << units.density_SI << " kg/m³" << std::setw(29) << " " << "║\n";
        std::cout << "╚══════════════════════════════════════════════════════════════════╝\n";
        std::cout << std::fixed;  // Reset formatting
    }
};

}  // namespace sph

#endif  // DISK_INITIALIZATION_PARAMS_HPP
```

### Step 2: Usage in disk_relaxation.cpp

```cpp
void initialize(std::shared_ptr<Simulation> sim,
               std::shared_ptr<SPHParameters> param) override {
    
    // User specifies input in pc/M☉, wants output in pc/M☉
    DiskInitializationParams params(
        /* R     */ 3.0,      // parsecs
        /* z     */ 0.2,      // parsecs
        /* M     */ 1000.0,   // solar masses
        /* n     */ 1.5,      // polytropic index
        /* nx,ny,nz */ 50, 50, 5,
        /* input  */ UnitSystemType::GALACTIC_PC,
        /* output */ UnitSystemType::GALACTIC_PC  // Can be different!
    );
    
    params.print();
    
    // ═══════════════════════════════════════════════════════════
    // From here: Work ONLY with params.code.* (dimensionless)
    // ═══════════════════════════════════════════════════════════
    
    const real R_disk = params.code.radius;        // = 1.0
    const real z_max = params.code.half_thickness; // ≈ 0.067
    const real M_total = params.code.total_mass;   // = 1.0
    
    // Grid in code units
    const real dx = (2.0 * R_disk) / params.code.grid_nx;
    const real dy = (2.0 * R_disk) / params.code.grid_ny;
    const real dz = (2.0 * z_max) / params.code.grid_nz;
    
    // Generate positions in code units
    std::vector<vec_t> positions_code;
    for (int ix = 0; ix < params.code.grid_nx; ++ix) {
        for (int iy = 0; iy < params.code.grid_ny; ++iy) {
            real x = -R_disk + (ix + 0.5) * dx;  // [-1, 1]
            real y = -R_disk + (iy + 0.5) * dy;  // [-1, 1]
            
            if (x*x + y*y > R_disk*R_disk) continue;
            
            for (int iz = 0; iz < params.code.grid_nz; ++iz) {
                real z = -z_max + (iz + 0.5) * dz;
                positions_code.push_back({x, y, z});
            }
        }
    }
    
    // Create particles in code units
    const real mpp = M_total / positions_code.size();
    std::vector<SPHParticle> particles;
    
    for (const auto &pos : positions_code) {
        // All physics in dimensionless units
        SPHParticle pp;
        pp.pos[0] = pos[0];  // Code units
        pp.pos[1] = pos[1];
        pp.pos[2] = pos[2];
        pp.mass = mpp;       // Code units
        // ... density, pressure from Lane-Emden (code units)
        
        particles.push_back(pp);
    }
    
    sim->set_particles(particles);
    param->gravity.constant = params.code.G;  // = 1.0
    
    // Store conversion info for output
    sim->set_unit_conversion_params(params);
}
```

### Step 3: CSV Output with Unit Conversion

```cpp
void writeCSV(const std::string &filename, 
              const std::vector<SPHParticle> &particles,
              const DiskInitializationParams &params,
              real time_code)
{
    std::ofstream csv(filename);
    
    // Header with output units
    csv << "time [" << params.units.output.time_unit << "],";
    csv << "id,";
    csv << "pos_x [" << params.units.output.length_unit << "],";
    csv << "pos_y [" << params.units.output.length_unit << "],";
    csv << "pos_z [" << params.units.output.length_unit << "],";
    csv << "mass [" << params.units.output.mass_unit << "],";
    csv << "dens [" << params.units.output.density_unit << "]\\n";
    
    // Convert data to output units
    real time_out = params.code_to_output_time(time_code);
    
    for (const auto &p : particles) {
        real x_out = params.code_to_output_length(p.pos[0]);
        real y_out = params.code_to_output_length(p.pos[1]);
        real z_out = params.code_to_output_length(p.pos[2]);
        real m_out = params.code_to_output_mass(p.mass);
        real rho_out = params.code_to_output_density(p.dens);
        
        csv << time_out << "," << p.id << ","
            << x_out << "," << y_out << "," << z_out << ","
            << m_out << "," << rho_out << "\\n";
    }
}
```

## Usage Examples

### Example 1: Astrophysics Standard (pc, M☉, Myr)

```cpp
DiskInitializationParams params(
    3.0, 0.2, 1000.0, 1.5, 50, 50, 5,
    UnitSystemType::GALACTIC_PC,  // Input
    UnitSystemType::GALACTIC_PC   // Output
);
```

Output CSV:
```
pos_x [pc], pos_y [pc], pos_z [pc], mass [M_sun], dens [M_sun/pc^3]
1.5, 0.3, 0.05, 0.101, 2.3e-5
...
```

### Example 2: Convert to SI for Analysis

```cpp
DiskInitializationParams params(
    3.0, 0.2, 1000.0, 1.5, 50, 50, 5,
    UnitSystemType::GALACTIC_PC,  // Input in pc/M☉
    UnitSystemType::SI            // Output in m/kg!
);
```

Output CSV:
```
pos_x [m], pos_y [m], pos_z [m], mass [kg], dens [kg/m^3]
4.629e16, 9.258e15, 1.543e15, 2.010e29, 2.450e-18
...
```

### Example 3: Galactic Scale (kpc, M☉, Gyr)

```cpp
DiskInitializationParams params(
    3.0, 0.3, 1e6, 1.5, 100, 100, 10,
    UnitSystemType::GALACTIC_KPC,  // Input
    UnitSystemType::GALACTIC_KPC   // Output
);
```

## Benefits

1. **Complete Flexibility**: Input ≠ Output units
2. **Numerical Stability**: Simulation always uses O(1) values
3. **User Friendly**: Specify parameters in natural units
4. **Analysis Ready**: Output in most convenient units for post-processing
5. **Type Safe**: Clear separation of input/code/output layers
6. **Leverages Existing Code**: Uses UnitFactory infrastructure
7. **Self-Documenting**: CSV headers show units automatically

## Migration Checklist

- [x] Design flexible parameter struct
- [ ] Implement header file `disk_initialization_params.hpp`
- [ ] Update `disk_relaxation.cpp` to use dimensionless simulation
- [ ] Modify CSV output to convert from code → user units
- [ ] Update config.json to specify G=1.0
- [ ] Test with different input/output unit combinations
- [ ] Verify no neighbor list overflow
- [ ] Document unit system choices in README
