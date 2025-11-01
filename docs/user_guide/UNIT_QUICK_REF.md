# Unit System Quick Reference

## Quick Start

```python
# Import
from analysis.units import UnitFactory
from analysis.readers import SimulationReader

# Auto-detect from simulation
reader = SimulationReader("path/to/output")
print(reader.units.name)  # "SI", "CGS", etc.

# Create unit systems
si = UnitFactory.create_si()
cgs = UnitFactory.create_cgs()
gal_kpc = UnitFactory.create_galactic_kpc()
gal_pc = UnitFactory.create_galactic_pc()

# Convert values
value_cgs = cgs.from_si(value_si, 'density')
value_si = cgs.to_si(value_cgs, 'density')
```

## Quantity Types

Use these strings for conversion:
- `'time'` - temporal
- `'length'` - spatial distance
- `'mass'` - inertial mass
- `'velocity'` - speed
- `'density'` - mass/volume
- `'pressure'` - force/area
- `'energy'` - specific energy (per unit mass)

## Unit Systems at a Glance

| System | Time | Length | Mass | When to Use |
|--------|------|--------|------|-------------|
| **SI** | s | m | kg | General, benchmarks |
| **CGS** | s | cm | g | Lab physics, literature |
| **Gal (kpc)** | Myr | kpc | M☉ | Galaxies, cosmology |
| **Gal (pc)** | Myr | pc | M☉ | Star clusters, GMCs |

## Physical Constants

```python
from analysis.units import PhysicalConstants as PC

PC.G      # 6.674e-11 m³ kg⁻¹ s⁻²
PC.M_sun  # 1.988e30 kg
PC.pc     # 3.086e16 m
PC.kpc    # 3.086e19 m
PC.Myr    # 3.156e13 s
PC.Gyr    # 3.156e16 s
PC.km     # 1000 m
PC.cm     # 0.01 m
PC.g      # 0.001 kg
```

## Common Conversions

### SI ↔ CGS
```python
cgs = UnitFactory.create_cgs()

# 1 m = 100 cm
length_cm = cgs.from_si(1.0, 'length')  # 100

# 1 kg = 1000 g
mass_g = cgs.from_si(1.0, 'mass')  # 1000

# 1 kg/m³ = 0.001 g/cm³
dens_gcm3 = cgs.from_si(1.0, 'density')  # 0.001

# 1 Pa = 10 dyne/cm²
pres_dyne = cgs.from_si(1.0, 'pressure')  # 10
```

### SI ↔ Galactic (kpc)
```python
gal = UnitFactory.create_galactic_kpc()

# 1 m = 3.24e-20 kpc
length_kpc = gal.from_si(1.0, 'length')

# 1 kg = 5.03e-31 M☉
mass_msun = gal.from_si(1.0, 'mass')

# 1 s = 3.17e-14 Myr
time_myr = gal.from_si(1.0, 'time')

# Typical galaxy:
# 10 kpc = 3.09e20 m
radius_m = gal.to_si(10.0, 'length')

# 1e11 M☉ = 1.99e41 kg
mass_kg = gal.to_si(1e11, 'mass')
```

## Example Workflows

### Shock Tube (SI → CGS)
```python
reader = SimulationReader("../build/results/DISPH/shock_tube/1D")
snap = reader.read_snapshot(0)
cgs = UnitFactory.create_cgs()

time_s = snap.time  # 0.0 s
time_s_cgs = cgs.from_si(time_s, 'time')  # 0.0 s (same)

dens_kgm3 = snap.dens[0]  # 1.0 kg/m³
dens_gcm3 = cgs.from_si(dens_kgm3, 'density')  # 0.001 g/cm³

pres_pa = snap.pres[0]  # 1.0 Pa
pres_dyne = cgs.from_si(pres_pa, 'pressure')  # 10 dyne/cm²
```

### Galaxy Setup (Physical → Code)
```python
gal = UnitFactory.create_galactic_kpc()

# Physical parameters
radius_kpc = 20.0     # disk radius
mass_msun = 1e11      # total mass
duration_myr = 5000.0 # simulation time

# Convert to SI for code
radius_m = gal.to_si(radius_kpc, 'length')    # 6.17e20 m
mass_kg = gal.to_si(mass_msun, 'mass')        # 1.99e41 kg
time_s = gal.to_si(duration_myr, 'time')      # 1.58e17 s

# Use in simulation setup...
```

## Automatic Detection

CSV headers determine units:

```
time [s],pos_x [m],... → SI
time [s],pos_x [cm],... → CGS
time [Myr],pos_x [kpc],... → Galactic (kpc)
time [Myr],pos_x [pc],... → Galactic (pc)
```

```python
reader = SimulationReader("output/")
snap = reader.read_snapshot(0)

# Units attached to snapshot
print(f"System: {snap.units.name}")
print(f"Time: {snap.time} {snap.units.time_unit}")
print(f"Density: {snap.dens[0]} {snap.units.density_unit}")
```

## Typical Values (for Reference)

### Shock Tube Benchmark
- Domain: -0.5 to 0.5 m
- Density: 0.125 - 1.0 kg/m³
- Pressure: 0.1 - 1.0 Pa
- Duration: ~0.2 s
- **Units**: SI

### Galaxy Simulation
- Disk radius: 10-30 kpc
- Disk mass: 10¹⁰-10¹² M☉
- Simulation time: 1-10 Gyr
- Velocity: ~100-300 km/s
- **Units**: Galactic (kpc)

### Star Cluster
- Size: 1-100 pc
- Mass: 10³-10⁶ M☉
- Age: 1-1000 Myr
- **Units**: Galactic (pc)

## Testing

```bash
cd analysis
python3 example_units.py
```

Shows:
- All unit system conversions
- Physical constants
- Auto-detection
- Conversion workflows

## Troubleshooting

**Problem**: Units not detected  
**Solution**: Check CSV header has `[unit]` brackets

**Problem**: Conversion gives unexpected result  
**Solution**: Check quantity type string (`'length'`, `'mass'`, etc.)

**Problem**: Want custom units  
**Solution**: Use `UnitFactory.createCustom()` or define your own

## Files

- **`UNIT_SYSTEM.md`** - Complete documentation
- **`analysis/units.py`** - Python implementation
- **`analysis/example_units.py`** - Examples & tests
- **`include/utilities/unit_conversions.hpp`** - C++ implementation
