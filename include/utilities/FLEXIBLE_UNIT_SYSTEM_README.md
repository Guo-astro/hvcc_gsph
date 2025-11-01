# Flexible Unit System for SPH Simulations

## Overview

The **Flexible Unit System** provides a robust, user-friendly way to work with physical units in SPH simulations. It automatically handles conversions between user-defined input units, dimensionless simulation units, and user-defined output units.

**Key Feature**: Input and output units can be completely different!

## Three-Layer Architecture

```
┌─────────────────┐
│  INPUT LAYER    │  User specifies parameters in convenient units
│  (pc, M☉, kpc)  │  Example: R = 3.0 pc, M = 1000 M☉
└────────┬────────┘
         │ Automatic conversion
         ↓
┌─────────────────┐
│ SIMULATION      │  Code always uses dimensionless units
│ (R=1, M=1, G=1) │  Guarantees numerical stability: all values O(1)
└────────┬────────┘
         │ Automatic conversion
         ↓
┌─────────────────┐
│ OUTPUT LAYER    │  User chooses output units (can differ from input!)
│ (SI, CGS, etc.) │  Example: Save in meters and kilograms
└─────────────────┘
```

## Quick Start

### Basic Usage

```cpp
#include "utilities/flexible_unit_params.hpp"

// Create unit system: input in pc/M☉, output in SI
sph::FlexibleUnitParams params(
    3.0,                              // Characteristic length: 3 pc
    1000.0,                           // Characteristic mass: 1000 M☉
    UnitSystemType::GALACTIC_PC,      // Input units
    UnitSystemType::SI                // Output units
);

// Show configuration
params.print();

// Use in simulation (dimensionless)
real R_code = params.code.length;     // = 1.0 (normalized)
real M_code = params.code.mass;       // = 1.0 (normalized)
real G_code = params.code.G;          // = 1.0 (normalized)

// Convert simulation results to output units
real x_output = params.code_to_output_length(x_simulation);
real v_output = params.code_to_output_velocity(v_simulation);
real rho_output = params.code_to_output_density(rho_simulation);
```

### Available Unit Systems

```cpp
enum class UnitSystemType {
    DIMENSIONLESS,   // Pure dimensionless (1, 1, 1)
    SI,              // m, kg, s
    CGS,             // cm, g, s
    GALACTIC_PC,     // pc, M☉, yr
    GALACTIC_KPC,    // kpc, M☉, Myr
    CUSTOM           // User-defined
};
```

## Complete Example: Disk Initialization

```cpp
#include "utilities/flexible_unit_params.hpp"

void initialize_disk() {
    // User wants to input in astronomical units, output in SI
    sph::FlexibleUnitParams units(
        3.0,                           // 3 pc radius
        1000.0,                        // 1000 solar masses
        UnitSystemType::GALACTIC_PC,   // Input: pc, M☉, yr
        UnitSystemType::SI             // Output: m, kg, s
    );
    
    units.print();  // Show all conversions
    
    // Input parameters (in pc, M☉, etc.)
    real R_disk_pc = 3.0;
    real M_disk_Msun = 1000.0;
    real cs_kmps = 0.2;  // Sound speed in km/s
    
    // Convert to code units for simulation
    real R_disk_code = units.input_to_code_length(R_disk_pc);
    real M_disk_code = units.input_to_code_mass(M_disk_Msun);
    real cs_code = units.input_to_code_velocity(cs_kmps);
    
    std::cout << "Simulation uses dimensionless units:\n";
    std::cout << "  R_disk = " << R_disk_code << " (normalized to 1.0)\n";
    std::cout << "  M_disk = " << M_disk_code << " (normalized to 1.0)\n";
    std::cout << "  c_s = " << cs_code << " (O(1) value)\n";
    
    // ... Run simulation with dimensionless quantities ...
    
    // Convert results for output (SI units)
    real x_particle_m = units.code_to_output_length(x_particle_code);
    real v_particle_ms = units.code_to_output_velocity(v_particle_code);
    real rho_particle_kgm3 = units.code_to_output_density(rho_particle_code);
    
    // Write to CSV with proper headers
    std::ofstream file("particles.csv");
    file << "x [m], y [m], z [m], vx [m/s], vy [m/s], vz [m/s], density [kg/m³]\n";
    // ... write converted data ...
}
```

## Why Dimensionless Simulation?

**Numerical Stability**: When working with astrophysical scales:
- Lengths: 10¹⁶ m (parsecs)
- Masses: 10³⁰ kg (solar masses)
- Times: 10¹³ s (years)

Directly using these values causes:
- **Overflow/underflow** in floating-point arithmetic
- **Loss of precision** in comparisons (neighbor finding, tree building)
- **Instabilities** in time integration

**Solution**: Normalize to O(1) values internally:
```
R = 1, M = 1, G = 1
→ All positions ~ 1, all velocities ~ 1, all densities ~ 1
```

This is why the simulation layer *always* uses dimensionless units!

## Conversion Functions

### Input → Code (Dimensionless)

```cpp
real code_length = params.input_to_code_length(length_input);
real code_mass = params.input_to_code_mass(mass_input);
real code_time = params.input_to_code_time(time_input);
real code_velocity = params.input_to_code_velocity(velocity_input);
real code_density = params.input_to_code_density(density_input);
```

### Code (Dimensionless) → Output

```cpp
real output_length = params.code_to_output_length(length_code);
real output_mass = params.code_to_output_mass(mass_code);
real output_time = params.code_to_output_time(time_code);
real output_velocity = params.code_to_output_velocity(velocity_code);
real output_density = params.code_to_output_density(density_code);
real output_energy = params.code_to_output_energy(energy_code);
real output_pressure = params.code_to_output_pressure(pressure_code);
```

### Output → Code (For reading checkpoints)

```cpp
real code_length = params.output_to_code_length(length_output);
real code_mass = params.output_to_code_mass(mass_output);
real code_density = params.output_to_code_density(density_output);
```

## Advanced: Different Input/Output Units

One of the most powerful features is **independent input and output units**:

```cpp
// Input in kpc/M☉ (good for large galactic simulations)
// Output in pc/M☉ (easier to visualize)
sph::FlexibleUnitParams params(
    10.0,                          // 10 kpc characteristic scale
    1e9,                           // 1 billion solar masses
    UnitSystemType::GALACTIC_KPC,  // Input: kpc
    UnitSystemType::GALACTIC_PC    // Output: pc (1000x smaller!)
);

// The simulation still uses R=1, M=1 internally
// But users see results in convenient units
```

**Use Cases**:
- Input in kpc (easy problem setup), output in pc (easier visualization)
- Input in CGS (legacy code), output in SI (modern standards)
- Input in observational units, output in SI for analysis tools

## CSV Output Example

The unit system makes it easy to write self-documenting output:

```cpp
void write_particles(const std::vector<Particle>& particles, 
                     const FlexibleUnitParams& units) {
    std::ofstream csv("particles.csv");
    
    // Self-documenting header with units
    csv << "x [" << units.units.output.length_unit << "], "
        << "y [" << units.units.output.length_unit << "], "
        << "z [" << units.units.output.length_unit << "], "
        << "vx [" << units.units.output.length_unit << "/" 
                  << units.units.output.time_unit << "], "
        << "density [" << units.units.output.density_unit << "]\n";
    
    for (const auto& p : particles) {
        csv << units.code_to_output_length(p.pos[0]) << ", "
            << units.code_to_output_length(p.pos[1]) << ", "
            << units.code_to_output_length(p.pos[2]) << ", "
            << units.code_to_output_velocity(p.vel[0]) << ", "
            << units.code_to_output_density(p.rho) << "\n";
    }
}
```

Output:
```csv
x [m], y [m], z [m], vx [m/s], density [kg/m³]
9.2537e+15, 1.8507e+16, 0.0, 1250.3, 3.14e-18
...
```

## Technical Details

### Characteristic Scales

The system internally computes characteristic scales in SI:

```
L_SI = input_length / input_length_factor
M_SI = input_mass / input_mass_factor
T_SI = sqrt(L³ / (G·M))          ← Derived from dynamical time
V_SI = L / T                      ← Derived velocity
ρ_SI = M / L³                     ← Derived density
```

All conversions go through SI as an intermediate step:
```
Input → SI → Code (dimensionless)
Code (dimensionless) → SI → Output
```

### Example Calculation

For R = 3 pc, M = 1000 M☉:

```
L_SI = 3.0 / 3.086e16 = 9.26e16 m
M_SI = 1000 / 1.989e30 = 1.99e33 kg
T_SI = sqrt(L³/(G·M)) = sqrt((9.26e16)³ / (6.67e-11 · 1.99e33))
     ≈ 2.40e13 s  (≈ 760,000 years)
V_SI = L/T = 9.26e16 / 2.40e13 ≈ 3860 m/s
```

Simulation uses:
- Positions in units of L_SI
- Velocities in units of V_SI
- All quantities O(1)

## Best Practices

### 1. Always Print Configuration

```cpp
params.print();  // Shows all conversions, catches errors early
```

### 2. Choose Input Units for Problem Setup

Use units that make your problem parameters natural:
- Disk radius 3 pc → Use GALACTIC_PC
- Galaxy 10 kpc → Use GALACTIC_KPC
- Lab experiment → Use SI or CGS

### 3. Choose Output Units for Analysis

Use units convenient for your analysis tools:
- Python/NumPy prefers SI
- Astronomy papers use pc/M☉
- Comparisons with observations use specific units

### 4. Trust Dimensionless Simulation

Never worry about:
- "Is my time step too small in seconds?"
- "Are my positions in meters or parsecs?"

The code handles it! Just work with O(1) values.

## Integration with Existing Code

The system works with existing `UnitFactory`:

```cpp
// Old approach (manual conversions)
UnitSystem units = UnitFactory::create(UnitSystemType::GALACTIC_PC);
real x_SI = x_pc / units.length_factor;
real v_SI = v_kmps * 1000.0;  // Manual km/s → m/s

// New approach (automatic)
FlexibleUnitParams params(3.0, 1000.0, 
                          UnitSystemType::GALACTIC_PC,
                          UnitSystemType::SI);
real x_SI = params.code_to_output_length(x_code);
real v_SI = params.code_to_output_velocity(v_code);
```

Both systems coexist! Use `FlexibleUnitParams` for new workflows.

## Debugging

The `print()` method shows all conversions:

```
╔══════════════════════════════════════════════════════════════════╗
║            Flexible Unit System Configuration                   ║
╠══════════════════════════════════════════════════════════════════╣
║ INPUT (User-Specified):                                          ║
║   Unit system:     Galactic (pc)                                 ║
║   Char. length:    3          pc                                 ║
║   Char. mass:      1000       M☉                                 ║
╠══════════════════════════════════════════════════════════════════╣
║ SIMULATION (Code Units - Dimensionless):                        ║
║   Length:          1                                             ║
║   Mass:            1                                             ║
║   G:               1                                             ║
╠══════════════════════════════════════════════════════════════════╣
║ OUTPUT (User-Specified):                                         ║
║   Unit system:     SI                                            ║
║   Length unit:     m                                             ║
║   Mass unit:       kg                                            ║
║   Density unit:    kg/m³                                         ║
╠══════════════════════════════════════════════════════════════════╣
║ CHARACTERISTIC SCALES (Internal - SI):                           ║
║   [L] = 9.2611e+16 m                                             ║
║   [M] = 1.9885e+33 kg                                            ║
║   [T] = 2.4015e+13 s                                             ║
║   [V] = 3.8568e+03 m/s                                           ║
║   [ρ] = 2.5062e-17 kg/m³                                         ║
╚══════════════════════════════════════════════════════════════════╝
```

Use this to verify:
- ✅ Input units correct?
- ✅ Output units what you want?
- ✅ Characteristic scales reasonable?

## Summary

**The Flexible Unit System solves three problems:**

1. **User Convenience**: Input in natural units (pc, M☉)
2. **Numerical Stability**: Simulate in dimensionless O(1) units
3. **Output Flexibility**: Save in any desired units (even different from input!)

**One-line benefit**: You never have to think about unit conversions again!

---

*For implementation details, see `include/utilities/flexible_unit_params.hpp`*
