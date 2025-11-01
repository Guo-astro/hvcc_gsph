#!/usr/bin/env python3
"""
Example: Using the unit conversion system for SPH simulations
"""

import sys
from pathlib import Path

# Add analysis directory to path
sys.path.insert(0, str(Path(__file__).parent))

from units import UnitFactory, UnitSystem, PhysicalConstants
from readers import SimulationReader

def example_predefined_units():
    """Demonstrate predefined unit systems"""
    print("="*80)
    print("PREDEFINED UNIT SYSTEMS")
    print("="*80)
    
    # Create different unit systems
    systems = {
        'Dimensionless': UnitFactory.create_dimensionless(),
        'SI': UnitFactory.create_si(),
        'CGS': UnitFactory.create_cgs(),
        'Galactic (kpc)': UnitFactory.create_galactic_kpc(),
        'Galactic (pc)': UnitFactory.create_galactic_pc(),
    }
    
    # Example value in SI: 1 meter
    length_si = 1.0  # m
    
    print(f"\nConverting {length_si} m to different unit systems:")
    for name, units in systems.items():
        converted = units.from_si(length_si, 'length')
        unit_label = units.get_unit_label('length')
        print(f"  {name:20s}: {converted:.6e} {unit_label}")
    
    print("\n" + "="*80)


def example_physical_constants():
    """Show physical constants available"""
    print("\nPHYSICAL CONSTANTS (SI units)")
    print("="*80)
    print(f"  Gravitational constant G: {PhysicalConstants.G:.6e} m³ kg⁻¹ s⁻²")
    print(f"  Solar mass M☉:           {PhysicalConstants.M_sun:.6e} kg")
    print(f"  Parsec (pc):              {PhysicalConstants.pc:.6e} m")
    print(f"  Kiloparsec (kpc):         {PhysicalConstants.kpc:.6e} m")
    print(f"  Megayear (Myr):           {PhysicalConstants.Myr:.6e} s")
    print(f"  Gigayear (Gyr):           {PhysicalConstants.Gyr:.6e} s")
    print("="*80)


def example_galactic_units():
    """Demonstrate galactic unit conversions"""
    print("\nGALACTIC UNITS EXAMPLE")
    print("="*80)
    
    # Create galactic unit system
    gal_units = UnitFactory.create_galactic_kpc()
    
    # Example: Milky Way disk
    radius_kpc = 15.0  # kpc
    mass_msun = 6e10   # M☉
    time_myr = 1000.0  # Myr
    
    # Convert to SI
    radius_m = gal_units.to_si(radius_kpc, 'length')
    mass_kg = gal_units.to_si(mass_msun, 'mass')
    time_s = gal_units.to_si(time_myr, 'time')
    
    print(f"\nMilky Way disk example:")
    print(f"  Radius: {radius_kpc} kpc = {radius_m:.3e} m")
    print(f"  Mass:   {mass_msun:.1e} M☉ = {mass_kg:.3e} kg")
    print(f"  Time:   {time_myr} Myr = {time_s:.3e} s = {time_s/PhysicalConstants.yr:.1f} yr")
    
    # Calculate density
    import numpy as np
    volume_m3 = (4/3) * np.pi * radius_m**3
    density_kg_m3 = mass_kg / volume_m3
    density_gal = gal_units.from_si(density_kg_m3, 'density')
    
    print(f"\n  Average density:")
    print(f"    {density_kg_m3:.3e} kg/m³")
    print(f"    {density_gal:.3e} {gal_units.get_unit_label('density')}")
    
    print("="*80)


def example_auto_detection():
    """Demonstrate automatic unit detection from simulation output"""
    print("\nAUTOMATIC UNIT DETECTION")
    print("="*80)
    
    # Path to shock tube results
    output_dir = "../build/results/DISPH/shock_tube/1D"
    if not Path(output_dir).exists():
        print(f"  ⚠ Directory not found: {output_dir}")
        print("  Run shock tube simulation first!")
        return
    
    # Read simulation
    reader = SimulationReader(output_dir)
    
    print(f"\nDetected from simulation output:")
    print(f"  Directory: {output_dir}")
    print(f"  Dimension: {reader.dim}D")
    print(f"  Snapshots: {reader.num_snapshots}")
    
    if reader.units:
        print(f"  Unit system: {reader.units.name}")
        print(f"    Time unit:     {reader.units.time_unit}")
        print(f"    Length unit:   {reader.units.length_unit}")
        print(f"    Mass unit:     {reader.units.mass_unit}")
        print(f"    Density unit:  {reader.units.density_unit}")
        print(f"    Pressure unit: {reader.units.pressure_unit}")
        print(f"    Energy unit:   {reader.units.energy_unit}")
    else:
        print("  Unit system: Not detected")
    
    # Read a snapshot
    snap = reader.read_snapshot(0)
    print(f"\nSnapshot 0:")
    print(f"  Time: {snap.time} {snap.units.time_unit if snap.units else ''}")
    print(f"  Particles: {snap.num_particles}")
    print(f"  Total mass: {snap.total_mass():.6f} {snap.units.mass_unit if snap.units else ''}")
    
    print("="*80)


def example_unit_conversion_workflow():
    """Show typical workflow for unit conversions"""
    print("\nTYPICAL WORKFLOW: Converting Shock Tube to CGS")
    print("="*80)
    
    output_dir = "../build/results/DISPH/shock_tube/1D"
    if not Path(output_dir).exists():
        print(f"  ⚠ Directory not found: {output_dir}")
        return
    
    # Read simulation (in SI units)
    reader = SimulationReader(output_dir)
    snap = reader.read_snapshot(reader.num_snapshots - 1)  # Last snapshot
    
    # Create target unit system
    cgs_units = UnitFactory.create_cgs()
    
    print(f"\nOriginal (SI):")
    print(f"  Time:     {snap.time:.6f} {snap.units.time_unit}")
    print(f"  Density:  {snap.dens[0]:.6f} {snap.units.density_unit}")
    print(f"  Pressure: {snap.pres[0]:.6f} {snap.units.pressure_unit}")
    
    # Convert
    if snap.units:
        time_cgs = cgs_units.from_si(snap.units.to_si(snap.time, 'time'), 'time')
        dens_cgs = cgs_units.from_si(snap.units.to_si(snap.dens[0], 'density'), 'density')
        pres_cgs = cgs_units.from_si(snap.units.to_si(snap.pres[0], 'pressure'), 'pressure')
        
        print(f"\nConverted (CGS):")
        print(f"  Time:     {time_cgs:.6f} {cgs_units.time_unit}")
        print(f"  Density:  {dens_cgs:.6e} {cgs_units.density_unit}")
        print(f"  Pressure: {pres_cgs:.6e} {cgs_units.pressure_unit}")
    
    print("="*80)


def main():
    """Run all examples"""
    example_predefined_units()
    example_physical_constants()
    example_galactic_units()
    example_auto_detection()
    example_unit_conversion_workflow()
    
    print("\n" + "="*80)
    print("USAGE SUMMARY")
    print("="*80)
    print("""
1. Create unit system:
   >>> from units import UnitFactory
   >>> units = UnitFactory.create_cgs()
   
2. Convert values:
   >>> value_si = 1.0  # meters
   >>> value_cgs = units.from_si(value_si, 'length')  # centimeters
   
3. Auto-detect from simulation:
   >>> from readers import SimulationReader
   >>> reader = SimulationReader("path/to/output")
   >>> print(reader.units.name)  # "SI", "CGS", etc.
   
4. Access physical constants:
   >>> from units import PhysicalConstants
   >>> M_sun_kg = PhysicalConstants.M_sun
    """)
    print("="*80)


if __name__ == "__main__":
    main()
