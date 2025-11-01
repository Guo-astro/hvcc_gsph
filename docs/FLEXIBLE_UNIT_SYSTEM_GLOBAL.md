# Flexible Unit System - Global Implementation Complete ✅

## Summary

The **Flexible Unit System** is now available globally for all simulation workflows!

## What Changed

### 1. New Global Header
**Location**: `include/utilities/flexible_unit_params.hpp`

**Features**:
- ✅ 3-layer architecture: Input → Code (dimensionless) → Output
- ✅ Input and output units can be completely different
- ✅ Automatic conversions via characteristic scales
- ✅ Self-documenting with `print()` method
- ✅ Works with all existing `UnitSystemType` options

### 2. Comprehensive Documentation
**Location**: `include/utilities/FLEXIBLE_UNIT_SYSTEM_README.md`

**Contents**:
- Philosophy and architecture
- Quick start guide
- Complete API reference
- Multiple usage examples
- Best practices
- Integration guide

### 3. Updated Workflows
- Disk relaxation workflow updated to use global header
- Include path changed to `#include "utilities/flexible_unit_params.hpp"`
- Old workflow-specific header preserved with redirect notice

## Quick Usage Example

```cpp
#include "utilities/flexible_unit_params.hpp"

// Create flexible unit system
sph::FlexibleUnitParams params(
    3.0,                              // 3 pc characteristic length
    1000.0,                           // 1000 M☉ characteristic mass
    UnitSystemType::GALACTIC_PC,      // Input in pc/M☉
    UnitSystemType::SI                // Output in m/kg
);

// Show configuration (highly recommended!)
params.print();

// Simulation uses dimensionless units
real R_disk = params.code.length;     // = 1.0 (normalized)
real M_disk = params.code.mass;       // = 1.0 (normalized)

// Convert for output
real x_meters = params.code_to_output_length(x_code);
real v_ms = params.code_to_output_velocity(v_code);
```

## Benefits for All Workflows

### Numerical Stability
- All simulations use O(1) dimensionless values internally
- No overflow/underflow from astrophysical scales (10¹⁶ m, 10³⁰ kg)
- Consistent precision across all operations

### User Flexibility
- Input parameters in natural units (pc, M☉, kpc, etc.)
- Output in any desired units (SI, CGS, GALACTIC, etc.)
- Input ≠ Output allowed!

### Self-Documenting Code
```cpp
params.print();
```
Shows complete conversion chain:
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
╚══════════════════════════════════════════════════════════════════╝
```

## Available Unit Systems

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

## Migration Guide for Existing Workflows

### Old Approach (Manual Conversions)
```cpp
#include "utilities/unit_conversions.hpp"

// Scattered conversions throughout code
real R_SI = R_pc * PhysicalConstants::pc_SI;
real M_SI = M_Msun * PhysicalConstants::M_sun_SI;
real T_SI = std::sqrt(R_SI * R_SI * R_SI / (G_SI * M_SI));
// ... many more manual conversions ...
```

### New Approach (Automatic)
```cpp
#include "utilities/flexible_unit_params.hpp"

// One-time setup
sph::FlexibleUnitParams params(R_pc, M_Msun, 
                               UnitSystemType::GALACTIC_PC,
                               UnitSystemType::SI);
params.print();  // Verify configuration

// All conversions handled automatically
real x_SI = params.code_to_output_length(x_code);
```

**Benefits**:
- ✅ No manual scale calculations
- ✅ No scattered conversion factors
- ✅ No unit confusion errors
- ✅ Complete visibility via `print()`

## Implementation Details

### Three Layers

**Layer 1: Input**
- User specifies parameters in convenient units
- Example: R = 3 pc, M = 1000 M☉

**Layer 2: Simulation (Always Dimensionless)**
- Code normalizes to R = 1, M = 1, G = 1
- All values O(1) for numerical stability
- Guaranteed consistent precision

**Layer 3: Output**
- Convert to any desired units
- Can differ from input units
- Self-documenting CSV headers

### Conversion Path

```
Input → SI (intermediate) → Code (dimensionless)
Code (dimensionless) → SI (intermediate) → Output
```

All conversions go through SI as a universal intermediate representation.

### Characteristic Scales

Automatically computed:
```cpp
units.length_SI = L_input / input.length_factor;
units.mass_SI = M_input / input.mass_factor;
units.time_SI = sqrt(L³ / (G·M));           // Dynamical time
units.velocity_SI = L / T;
units.density_SI = M / L³;
units.energy_SI = V²;                       // Specific energy
units.pressure_SI = M / (L·T²);
```

## File Locations

```
include/utilities/
├── flexible_unit_params.hpp           # Main header (use this!)
├── FLEXIBLE_UNIT_SYSTEM_README.md     # Complete documentation
├── unit_conversions.hpp               # Legacy UnitFactory (still used internally)
└── unit_system.hpp                    # UnitSystem struct

simulations/workflows/razor_thin_hvcc_gdisph_workflow/01_relaxation/
├── disk_relaxation.cpp                # Updated to use global header
└── FLEXIBLE_UNIT_SYSTEM_DESIGN.md     # Redirect notice to global docs
```

## Build Status

✅ Clean rebuild successful  
✅ All tests passing  
✅ No compilation errors  
✅ Warnings are pre-existing (not from new code)  

## Next Steps

### For Workflow Developers

1. **Include the header**:
   ```cpp
   #include "utilities/flexible_unit_params.hpp"
   ```

2. **Create unit system**:
   ```cpp
   sph::FlexibleUnitParams units(
       characteristic_length,
       characteristic_mass,
       input_type,
       output_type
   );
   ```

3. **Always call `print()`** to verify setup:
   ```cpp
   units.print();
   ```

4. **Use dimensionless values** in simulation:
   ```cpp
   real R = units.code.length;  // = 1.0
   ```

5. **Convert for output**:
   ```cpp
   real x_out = units.code_to_output_length(x_code);
   ```

### For Code Reviews

Check for:
- ✅ `params.print()` called to show configuration
- ✅ CSV headers include units from `units.output.*_unit`
- ✅ No manual conversion factors (`* pc_SI`, `* M_sun_SI`)
- ✅ Simulation uses `params.code.*` (all = 1.0)

## Philosophy

**"Input in what makes sense, simulate in O(1), output in what you need"**

This approach:
- Reduces cognitive load (no unit tracking during development)
- Prevents numerical errors (overflow, underflow, precision loss)
- Maintains flexibility (input ≠ output allowed)
- Self-documents (print() shows complete conversion chain)

## Support

- **Complete guide**: `include/utilities/FLEXIBLE_UNIT_SYSTEM_README.md`
- **Header file**: `include/utilities/flexible_unit_params.hpp`
- **Example usage**: See `simulations/workflows/razor_thin_hvcc_gdisph_workflow/01_relaxation/disk_relaxation.cpp`

---

**Status**: ✅ **Ready for production use in all workflows**

**Last Updated**: 2025-01-XX  
**Version**: 1.0.0
