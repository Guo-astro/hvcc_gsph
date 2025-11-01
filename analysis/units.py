"""
Unit conversion utilities for SPH analysis tools.
Provides consistent unit handling between simulation output and analytical solutions.
"""

import numpy as np
from dataclasses import dataclass
from typing import Dict, Optional


@dataclass
class PhysicalConstants:
    """Physical constants in SI units"""
    G = 6.67430e-11  # m^3 kg^-1 s^-2 (Gravitational constant)
    c = 2.99792458e8  # m/s (Speed of light)
    M_sun = 1.98847e30  # kg (Solar mass)
    pc = 3.0856775814913673e16  # m (Parsec)
    kpc = 3.0856775814913673e19  # m (Kiloparsec)
    yr = 3.15576e7  # s (Year)
    Myr = 3.15576e13  # s (Megayear)
    Gyr = 3.15576e16  # s (Gigayear)
    km = 1e3  # m (Kilometer)
    cm = 1e-2  # m (Centimeter)
    g = 1e-3  # kg (Gram)


class UnitSystem:
    """
    Represents a physical unit system with conversion factors.
    
    All internal simulation values are assumed to be in SI units.
    Conversion factors convert FROM SI TO the target unit system.
    """
    
    def __init__(self, name: str = "SI"):
        self.name = name
        self.time_factor = 1.0
        self.length_factor = 1.0
        self.mass_factor = 1.0
        self.velocity_factor = 1.0
        self.density_factor = 1.0
        self.pressure_factor = 1.0
        self.energy_factor = 1.0
        
        self.time_unit = "s"
        self.length_unit = "m"
        self.mass_unit = "kg"
        self.velocity_unit = "m/s"
        self.density_unit = "kg/m³"
        self.pressure_unit = "Pa"
        self.energy_unit = "J/kg"
    
    def to_si(self, value: float, quantity_type: str) -> float:
        """Convert from this unit system to SI"""
        factor_map = {
            'time': 1.0 / self.time_factor,
            'length': 1.0 / self.length_factor,
            'mass': 1.0 / self.mass_factor,
            'velocity': 1.0 / self.velocity_factor,
            'density': 1.0 / self.density_factor,
            'pressure': 1.0 / self.pressure_factor,
            'energy': 1.0 / self.energy_factor,
        }
        return value * factor_map.get(quantity_type, 1.0)
    
    def from_si(self, value: float, quantity_type: str) -> float:
        """Convert from SI to this unit system"""
        factor_map = {
            'time': self.time_factor,
            'length': self.length_factor,
            'mass': self.mass_factor,
            'velocity': self.velocity_factor,
            'density': self.density_factor,
            'pressure': self.pressure_factor,
            'energy': self.energy_factor,
        }
        return value * factor_map.get(quantity_type, 1.0)
    
    def get_unit_label(self, quantity_type: str) -> str:
        """Get the unit label for a quantity type"""
        unit_map = {
            'time': self.time_unit,
            'length': self.length_unit,
            'mass': self.mass_unit,
            'velocity': self.velocity_unit,
            'density': self.density_unit,
            'pressure': self.pressure_unit,
            'energy': self.energy_unit,
        }
        return unit_map.get(quantity_type, "")


class UnitFactory:
    """Factory for creating predefined unit systems"""
    
    @staticmethod
    def create_dimensionless() -> UnitSystem:
        """Code units (no physical units)"""
        units = UnitSystem("Dimensionless")
        units.time_unit = ""
        units.length_unit = ""
        units.mass_unit = ""
        units.velocity_unit = ""
        units.density_unit = ""
        units.pressure_unit = ""
        units.energy_unit = ""
        return units
    
    @staticmethod
    def create_si() -> UnitSystem:
        """SI units (m, kg, s)"""
        return UnitSystem("SI")
    
    @staticmethod
    def create_cgs() -> UnitSystem:
        """CGS units (cm, g, s)"""
        units = UnitSystem("CGS")
        
        units.time_factor = 1.0  # s -> s
        units.length_factor = 1.0 / PhysicalConstants.cm  # m -> cm
        units.mass_factor = 1.0 / PhysicalConstants.g  # kg -> g
        units.velocity_factor = 1.0 / PhysicalConstants.cm  # m/s -> cm/s
        units.density_factor = (1.0 / PhysicalConstants.g) / (1.0 / PhysicalConstants.cm)**3
        units.pressure_factor = (1.0 / PhysicalConstants.g) / (1.0 / PhysicalConstants.cm)  # Pa -> dyne/cm²
        units.energy_factor = (1.0 / PhysicalConstants.cm)**2  # J/kg -> erg/g
        
        units.time_unit = "s"
        units.length_unit = "cm"
        units.mass_unit = "g"
        units.velocity_unit = "cm/s"
        units.density_unit = "g/cm³"
        units.pressure_unit = "dyne/cm²"
        units.energy_unit = "erg/g"
        
        return units
    
    @staticmethod
    def create_galactic_kpc() -> UnitSystem:
        """Galactic units (kpc, M_sun, Myr, km/s)"""
        units = UnitSystem("Galactic (kpc)")
        
        length_unit_m = PhysicalConstants.kpc
        mass_unit_kg = PhysicalConstants.M_sun
        time_unit_s = PhysicalConstants.Myr
        velocity_unit_ms = length_unit_m / time_unit_s
        
        units.time_factor = 1.0 / time_unit_s
        units.length_factor = 1.0 / length_unit_m
        units.mass_factor = 1.0 / mass_unit_kg
        units.velocity_factor = 1.0 / velocity_unit_ms
        units.density_factor = 1.0 / (mass_unit_kg / length_unit_m**3)
        units.pressure_factor = 1.0 / (mass_unit_kg / (length_unit_m * time_unit_s**2))
        units.energy_factor = 1.0 / velocity_unit_ms**2
        
        units.time_unit = "Myr"
        units.length_unit = "kpc"
        units.mass_unit = "M☉"
        units.velocity_unit = "kpc/Myr"
        units.density_unit = "M☉/kpc³"
        units.pressure_unit = "M☉/(kpc·Myr²)"
        units.energy_unit = "(kpc/Myr)²"
        
        return units
    
    @staticmethod
    def create_galactic_pc() -> UnitSystem:
        """Galactic units (pc, M_sun, Myr, km/s)"""
        units = UnitSystem("Galactic (pc)")
        
        length_unit_m = PhysicalConstants.pc
        mass_unit_kg = PhysicalConstants.M_sun
        time_unit_s = PhysicalConstants.Myr
        velocity_unit_ms = length_unit_m / time_unit_s
        
        units.time_factor = 1.0 / time_unit_s
        units.length_factor = 1.0 / length_unit_m
        units.mass_factor = 1.0 / mass_unit_kg
        units.velocity_factor = 1.0 / velocity_unit_ms
        units.density_factor = 1.0 / (mass_unit_kg / length_unit_m**3)
        units.pressure_factor = 1.0 / (mass_unit_kg / (length_unit_m * time_unit_s**2))
        units.energy_factor = 1.0 / velocity_unit_ms**2
        
        units.time_unit = "Myr"
        units.length_unit = "pc"
        units.mass_unit = "M☉"
        units.velocity_unit = "pc/Myr"
        units.density_unit = "M☉/pc³"
        units.pressure_unit = "M☉/(pc·Myr²)"
        units.energy_unit = "(pc/Myr)²"
        
        return units
    
    @staticmethod
    def detect_from_csv_header(header_line: str) -> UnitSystem:
        """
        Detect unit system from CSV header.
        
        Example header: "time [s],pos_x [m],vel_x [m/s],..."
        """
        if '[' not in header_line:
            return UnitFactory.create_dimensionless()
        
        # Extract time unit
        if '[Myr]' in header_line or 'Myr' in header_line:
            if '[kpc]' in header_line:
                return UnitFactory.create_galactic_kpc()
            elif '[pc]' in header_line:
                return UnitFactory.create_galactic_pc()
        
        if '[cm]' in header_line and '[g]' in header_line:
            return UnitFactory.create_cgs()
        
        if '[m]' in header_line and '[kg]' in header_line:
            return UnitFactory.create_si()
        
        # Default to SI
        return UnitFactory.create_si()


def convert_quantity_array(values: np.ndarray, quantity_type: str, 
                          from_units: UnitSystem, to_units: UnitSystem) -> np.ndarray:
    """
    Convert an array of quantities from one unit system to another.
    
    Args:
        values: Array of values to convert
        quantity_type: Type of quantity ('time', 'length', 'mass', etc.)
        from_units: Source unit system
        to_units: Target unit system
    
    Returns:
        Converted array
    """
    # Convert to SI first
    si_values = from_units.to_si(values, quantity_type)
    # Then convert to target units
    return to_units.from_si(si_values, quantity_type)


# Convenience functions for common conversions
def kpc_to_m(kpc: float) -> float:
    """Convert kiloparsecs to meters"""
    return kpc * PhysicalConstants.kpc


def m_to_kpc(m: float) -> float:
    """Convert meters to kiloparsecs"""
    return m / PhysicalConstants.kpc


def msun_to_kg(msun: float) -> float:
    """Convert solar masses to kilograms"""
    return msun * PhysicalConstants.M_sun


def kg_to_msun(kg: float) -> float:
    """Convert kilograms to solar masses"""
    return kg / PhysicalConstants.M_sun


def myr_to_s(myr: float) -> float:
    """Convert megayears to seconds"""
    return myr * PhysicalConstants.Myr


def s_to_myr(s: float) -> float:
    """Convert seconds to megayears"""
    return s / PhysicalConstants.Myr
