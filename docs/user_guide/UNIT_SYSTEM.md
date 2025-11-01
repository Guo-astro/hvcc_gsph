# Unit Conversion System Documentation

## Overview

The SPH code now includes a flexible unit conversion system that supports:
- **Dimensionless units** (code units)
- **SI units** (m, kg, s)
- **CGS units** (cm, g, s)
- **Galactic units** (kpc or pc, M☉, Myr)
- **Custom units** (user-defined)

## Architecture

### C++ Side (`include/utilities/`)

1. **`unit_system.hpp`** - Basic unit system structure
   - Stores conversion factors and unit labels
   - Used by Output class for CSV headers

2. **`unit_conversions.hpp`** - Enhanced factory system (NEW)
   - `PhysicalConstants` namespace - fundamental constants
   - `UnitSystemType` enum - predefined types
   - `UnitFactory` class - creates unit systems

### Python Side (`analysis/`)

1. **`units.py`** - Python unit conversion module (NEW)
   - `PhysicalConstants` - dataclass with SI constants
   - `UnitSystem` - unit conversion class
   - `UnitFactory` - factory for creating unit systems

2. **`readers.py`** - Updated to detect units
   - Auto-detects unit system from CSV headers
   - Attaches units to `ParticleSnapshot`

## Usage Examples

### C++ Usage

```cpp
#include "utilities/unit_conversions.hpp"

using namespace sph;

// Create SI units (default)
auto si_units = UnitFactory::create(UnitSystemType::SI);

// Create CGS units
auto cgs_units = UnitFactory::create(UnitSystemType::CGS);

// Create galactic units
auto gal_units = UnitFactory::create(UnitSystemType::GALACTIC_KPC);

// Create custom units
auto custom = UnitFactory::createCustom(
    1e3,      // length: 1 unit = 1 km = 1000 m
    1.0,      // mass: 1 unit = 1 kg
    3600.0,   // time: 1 unit = 1 hour = 3600 s
    "km", "kg", "hr"
);

// Use in solver
Solver solver;
solver.set_unit_system(gal_units);
```

### Python Usage

```python
from analysis.units import UnitFactory, PhysicalConstants
from analysis.readers import SimulationReader

# 1. Create unit systems
si = UnitFactory.create_si()
cgs = UnitFactory.create_cgs()
gal = UnitFactory.create_galactic_kpc()

# 2. Convert values
length_m = 1.0  # meters
length_cm = cgs.from_si(length_m, 'length')  # -> 100 cm
length_kpc = gal.from_si(length_m, 'length')  # -> 3.24e-20 kpc

# 3. Auto-detect from simulation
reader = SimulationReader("path/to/output")
print(f"Unit system: {reader.units.name}")
print(f"Time unit: {reader.units.time_unit}")

# 4. Access physical constants
M_sun = PhysicalConstants.M_sun  # kg
kpc_in_m = PhysicalConstants.kpc  # m

# 5. Convert snapshot data
snap = reader.read_snapshot(0)
time_si = snap.units.to_si(snap.time, 'time')
time_myr = gal.from_si(time_si, 'time')
```

## Supported Unit Systems

### 1. Dimensionless (Code Units)
No physical units - all values are pure numbers.

```python
units = UnitFactory.create_dimensionless()
# All factors = 1.0, all labels = ""
```

### 2. SI Units
International System of Units

| Quantity | Unit | Symbol |
|----------|------|--------|
| Time | second | s |
| Length | meter | m |
| Mass | kilogram | kg |
| Density | kg/m³ | kg/m³ |
| Pressure | Pascal | Pa |
| Energy | Joule/kg | J/kg |

```python
units = UnitFactory.create_si()
```

### 3. CGS Units
Centimeter-Gram-Second system

| Quantity | Unit | Conversion |
|----------|------|------------|
| Time | second | s |
| Length | centimeter | 1 m = 100 cm |
| Mass | gram | 1 kg = 1000 g |
| Density | g/cm³ | 1 kg/m³ = 0.001 g/cm³ |
| Pressure | dyne/cm² | 1 Pa = 10 dyne/cm² |
| Energy | erg/g | 1 J/kg = 10⁴ erg/g |

```python
units = UnitFactory.create_cgs()
```

### 4. Galactic Units (kpc)
Astrophysical units for galaxy simulations

| Quantity | Unit | Value in SI |
|----------|------|-------------|
| Time | Megayear (Myr) | 3.156×10¹³ s |
| Length | kiloparsec (kpc) | 3.086×10¹⁹ m |
| Mass | Solar mass (M☉) | 1.988×10³⁰ kg |
| Velocity | kpc/Myr | ≈ 978 km/s |
| Density | M☉/kpc³ | - |

```python
units = UnitFactory.create_galactic_kpc()

# Example: Milky Way disk
radius = 15.0  # kpc
mass = 6e10    # M☉
time = 1000.0  # Myr (1 Gyr)
```

### 5. Galactic Units (pc)
For smaller-scale astrophysical simulations

| Quantity | Unit | Value in SI |
|----------|------|-------------|
| Time | Megayear (Myr) | 3.156×10¹³ s |
| Length | parsec (pc) | 3.086×10¹⁶ m |
| Mass | Solar mass (M☉) | 1.988×10³⁰ kg |

```python
units = UnitFactory.create_galactic_pc()
```

## Physical Constants

Available in both C++ and Python:

```python
from analysis.units import PhysicalConstants

G = PhysicalConstants.G          # 6.674×10⁻¹¹ m³ kg⁻¹ s⁻²
M_sun = PhysicalConstants.M_sun  # 1.988×10³⁰ kg
pc = PhysicalConstants.pc        # 3.086×10¹⁶ m
kpc = PhysicalConstants.kpc      # 3.086×10¹⁹ m
Myr = PhysicalConstants.Myr      # 3.156×10¹³ s
Gyr = PhysicalConstants.Gyr      # 3.156×10¹⁶ s
km = PhysicalConstants.km        # 1000 m
```

## Automatic Unit Detection

The Python readers automatically detect units from CSV headers:

```
time [s],pos_x [m],vel_x [m/s],... → SI units
time [Myr],pos_x [kpc],vel_x [kpc/Myr],... → Galactic (kpc)
time [s],pos_x [cm],vel_x [cm/s],... → CGS units
```

## Integration with Simulations

### Setting Units in JSON Config

Add to your JSON configuration:

```json
{
    "SPHType": "disph",
    "gamma": 1.4,
    "unitSystem": "SI",  // or "CGS", "GALACTIC_KPC", etc.
    ...
}
```

### Output Files

CSV headers will include unit labels:

```
time [Myr],pos_x [kpc],pos_y [kpc],pos_z [kpc],vel_x [kpc/Myr],...
```

### Analysis Scripts

All analysis scripts automatically handle units:

```python
# Shock tube comparison
snap = reader.read_snapshot(5)
print(f"Time: {snap.time} {snap.units.time_unit}")
print(f"Density: {snap.dens[0]} {snap.units.density_unit}")

# Units are automatically preserved in comparisons
theory, error = TheoreticalComparison.compare_shock_tube(snap)
```

## Common Conversion Patterns

### 1. Convert Simulation Output

```python
# Read simulation in SI
reader = SimulationReader("output/")
snap = reader.read_snapshot(0)

# Convert to CGS
cgs = UnitFactory.create_cgs()
time_cgs = cgs.from_si(snap.time, 'time')
dens_cgs = cgs.from_si(snap.dens[0], 'density')
```

### 2. Set Up Galactic Simulation

```python
# Initial conditions in physical units
disk_radius_kpc = 20.0   # kpc
disk_mass_msun = 1e11    # M☉
simulation_time_myr = 5000.0  # Myr

gal = UnitFactory.create_galactic_kpc()

# Convert for use in code
radius_code = gal.to_si(disk_radius_kpc, 'length')
mass_code = gal.to_si(disk_mass_msun, 'mass')
time_code = gal.to_si(simulation_time_myr, 'time')
```

### 3. Compare Different Unit Systems

```python
# Same value in different systems
value_si = 1.0  # kg/m³

for name, units in [
    ('SI', UnitFactory.create_si()),
    ('CGS', UnitFactory.create_cgs()),
    ('Galactic', UnitFactory.create_galactic_kpc())
]:
    converted = units.from_si(value_si, 'density')
    label = units.get_unit_label('density')
    print(f"{name}: {converted:.3e} {label}")
```

## Testing

Run the example script:

```bash
cd analysis
python3 example_units.py
```

This will demonstrate:
- All predefined unit systems
- Physical constants
- Automatic detection
- Conversion workflows

## Future Enhancements

Potential additions:

1. **More unit systems**:
   - Atomic units
   - Planck units
   - Custom astrophysical systems

2. **Dimensional analysis**:
   - Automatic checking of dimensional consistency
   - Warning when mixing incompatible units

3. **Configuration files**:
   - Store unit preferences in config
   - Per-simulation unit settings

4. **Visualization**:
   - Automatic unit labeling in plots
   - Unit-aware comparison tools

## References

- SI units: [BIPM SI Brochure](https://www.bipm.org/en/publications/si-brochure/)
- Astrophysical units: [IAU recommendations](https://www.iau.org/publications/)
- Physical constants: [CODATA 2018](https://physics.nist.gov/cuu/Constants/)

---

**Note**: This unit system is designed to be flexible and extensible. If you need a custom unit system not covered here, use `UnitFactory::createCustom()` (C++) or define your own UnitSystem (Python).
