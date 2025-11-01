#ifndef DISK_INITIALIZATION_PARAMS_HPP
#define DISK_INITIALIZATION_PARAMS_HPP

#include "utilities/defines.hpp"
#include "utilities/unit_conversions.hpp"
#include <iostream>
#include <iomanip>
#include <cmath>

namespace sph {

/**
 * @brief Flexible disk initialization with user-defined input and output units
 * 
 * Design Philosophy:
 *   Input (user units) → Code (dimensionless) → Output (user units, can differ!)
 * 
 * Example:
 *   User inputs in [pc, M☉, Myr]
 *   →  Simulation runs with [R=1, M=1, G=1] (dimensionless, numerically stable)
 *   →  Output in [AU, M_earth, years] or any other unit system
 * 
 * This provides complete flexibility while ensuring numerical stability.
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
