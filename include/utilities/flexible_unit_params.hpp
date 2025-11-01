#ifndef FLEXIBLE_UNIT_PARAMS_HPP
#define FLEXIBLE_UNIT_PARAMS_HPP

/**
 * @file flexible_unit_params.hpp
 * @brief Flexible unit system for SPH simulations with user-defined input/output units
 * 
 * This provides a three-layer architecture:
 *   1. Input Layer:  User specifies parameters in convenient units (pc, M☉, kpc, etc.)
 *   2. Simulation Layer: Always dimensionless (R=1, M=1, G=1) for numerical stability
 *   3. Output Layer: User chooses output units (can differ from input!)
 * 
 * @example
 * ```cpp
 * // Create initialization parameters
 * FlexibleUnitParams params(
 *     3.0, 1000.0,  // 3 pc radius, 1000 M☉ mass
 *     UnitSystemType::GALACTIC_PC,  // Input in pc/M☉
 *     UnitSystemType::SI            // Output in m/kg
 * );
 * 
 * params.print();  // Show all conversions
 * 
 * // Use in simulation (dimensionless)
 * real R_code = params.code.length;  // = 1.0
 * 
 * // Convert for output
 * real x_output = params.code_to_output_length(x_code);
 * ```
 * 
 * @author SPH Code Team
 * @date 2025
 */

#include "utilities/defines.hpp"
#include "utilities/unit_conversions.hpp"
#include <iostream>
#include <iomanip>
#include <cmath>

namespace sph {

/**
 * @brief Flexible unit system parameters for SPH simulations
 * 
 * Provides automatic conversion between user-defined input units, 
 * dimensionless code units, and user-defined output units.
 */
class FlexibleUnitParams {
public:
    
    // ═══════════════════════════════════════════════════════════
    // Layer 1: User Input Parameters
    // ═══════════════════════════════════════════════════════════
    
    struct InputParams {
        real length;              ///< Characteristic length in input units
        real mass;                ///< Characteristic mass in input units
        UnitSystemType type;      ///< Input unit system type
    } input;
    
    // ═══════════════════════════════════════════════════════════
    // Layer 2: Code Units (Dimensionless)
    // ═══════════════════════════════════════════════════════════
    
    struct CodeParams {
        real length;              ///< Always 1.0 (normalized)
        real mass;                ///< Always 1.0 (normalized)
        real G;                   ///< Always 1.0 (normalized)
    } code;
    
    // ═══════════════════════════════════════════════════════════
    // Layer 3: Unit Conversion System
    // ═══════════════════════════════════════════════════════════
    
    struct ConversionFactors {
        // Characteristic scales (SI)
        real length_SI;           ///< Characteristic length in meters
        real mass_SI;             ///< Characteristic mass in kilograms
        real time_SI;             ///< Derived time: sqrt(L³/GM) in seconds
        real velocity_SI;         ///< Derived velocity: L/T in m/s
        real density_SI;          ///< Derived density: M/L³ in kg/m³
        real energy_SI;           ///< Derived specific energy: (L/T)² in J/kg
        real pressure_SI;         ///< Derived pressure: M/(L·T²) in Pa
        
        // Unit systems
        UnitSystem input;         ///< Input unit system (from UnitFactory)
        UnitSystem output;        ///< Output unit system (from UnitFactory)
        std::string input_name;   ///< Input system name (e.g., "Galactic (pc)")
        std::string output_name;  ///< Output system name (e.g., "SI")
    } units;
    
    // ═══════════════════════════════════════════════════════════
    // Constructors
    // ═══════════════════════════════════════════════════════════
    
    /**
     * @brief Default constructor (SI units)
     */
    FlexibleUnitParams() 
        : FlexibleUnitParams(1.0, 1.0, UnitSystemType::SI, UnitSystemType::SI) {}
    
    /**
     * @brief Constructor with flexible input/output units
     * 
     * @param L_in Characteristic length in input units
     * @param M_in Characteristic mass in input units
     * @param input_type Unit system for input (default: GALACTIC_PC)
     * @param output_type Unit system for output (default: same as input)
     */
    FlexibleUnitParams(
        real L_in, 
        real M_in,
        UnitSystemType input_type = UnitSystemType::GALACTIC_PC,
        UnitSystemType output_type = UnitSystemType::GALACTIC_PC)
    {
        using namespace PhysicalConstants;
        
        // Store input
        input.length = L_in;
        input.mass = M_in;
        input.type = input_type;
        
        // Create unit systems
        units.input = UnitFactory::create(input_type);
        units.output = UnitFactory::create(output_type);
        units.input_name = UnitFactory::getTypeName(input_type);
        units.output_name = UnitFactory::getTypeName(output_type);
        
        // Convert input → SI (characteristic scales)
        units.length_SI = L_in / units.input.length_factor;
        units.mass_SI = M_in / units.input.mass_factor;
        
        // Derive other scales
        units.time_SI = std::sqrt(units.length_SI * units.length_SI * units.length_SI 
                                  / (G_SI * units.mass_SI));
        units.velocity_SI = units.length_SI / units.time_SI;
        units.density_SI = units.mass_SI / std::pow(units.length_SI, 3);
        units.energy_SI = units.velocity_SI * units.velocity_SI;  // Specific energy
        units.pressure_SI = units.mass_SI / (units.length_SI * units.time_SI * units.time_SI);
        
        // Set code units (dimensionless)
        code.length = 1.0;
        code.mass = 1.0;
        code.G = 1.0;
    }
    
    // ═══════════════════════════════════════════════════════════
    // Input → Code Conversions
    // ═══════════════════════════════════════════════════════════
    
    real input_to_code_length(real L) const {
        return (L / units.input.length_factor) / units.length_SI;
    }
    
    real input_to_code_mass(real M) const {
        return (M / units.input.mass_factor) / units.mass_SI;
    }
    
    real input_to_code_time(real t) const {
        return (t / units.input.time_factor) / units.time_SI;
    }
    
    real input_to_code_velocity(real v) const {
        return (v * units.input.time_factor / units.input.length_factor) / units.velocity_SI;
    }
    
    real input_to_code_density(real rho) const {
        return (rho / units.input.density_factor) / units.density_SI;
    }
    
    // ═══════════════════════════════════════════════════════════
    // Code → Output Conversions
    // ═══════════════════════════════════════════════════════════
    
    real code_to_output_length(real L_code) const {
        real L_SI = L_code * units.length_SI;
        return L_SI * units.output.length_factor;
    }
    
    real code_to_output_mass(real M_code) const {
        real M_SI = M_code * units.mass_SI;
        return M_SI * units.output.mass_factor;
    }
    
    real code_to_output_time(real t_code) const {
        real t_SI = t_code * units.time_SI;
        return t_SI * units.output.time_factor;
    }
    
    real code_to_output_velocity(real v_code) const {
        real v_SI = v_code * units.velocity_SI;
        return v_SI * units.output.length_factor / units.output.time_factor;
    }
    
    real code_to_output_density(real rho_code) const {
        real rho_SI = rho_code * units.density_SI;
        return rho_SI * units.output.density_factor;
    }
    
    real code_to_output_energy(real e_code) const {
        real e_SI = e_code * units.energy_SI;
        return e_SI * units.output.energy_factor;
    }
    
    real code_to_output_pressure(real p_code) const {
        real p_SI = p_code * units.pressure_SI;
        return p_SI * units.output.pressure_factor;
    }
    
    // ═══════════════════════════════════════════════════════════
    // Output → Code Conversions (for reading initial conditions)
    // ═══════════════════════════════════════════════════════════
    
    real output_to_code_length(real L_out) const {
        real L_SI = L_out / units.output.length_factor;
        return L_SI / units.length_SI;
    }
    
    real output_to_code_mass(real M_out) const {
        real M_SI = M_out / units.output.mass_factor;
        return M_SI / units.mass_SI;
    }
    
    real output_to_code_density(real rho_out) const {
        real rho_SI = rho_out / units.output.density_factor;
        return rho_SI / units.density_SI;
    }
    
    // ═══════════════════════════════════════════════════════════
    // Display
    // ═══════════════════════════════════════════════════════════
    
    void print() const {
        std::cout << "\n";
        std::cout << "╔══════════════════════════════════════════════════════════════════╗\n";
        std::cout << "║            Flexible Unit System Configuration                   ║\n";
        std::cout << "╠══════════════════════════════════════════════════════════════════╣\n";
        std::cout << "║ INPUT (User-Specified):                                          ║\n";
        std::cout << "║   Unit system:     " << std::setw(44) << std::left << units.input_name << "║\n";
        std::cout << "║   Char. length:    " << std::setw(10) << input.length 
                  << " " << std::setw(33) << std::left << units.input.length_unit << "║\n";
        std::cout << "║   Char. mass:      " << std::setw(10) << input.mass 
                  << " " << std::setw(33) << std::left << units.input.mass_unit << "║\n";
        std::cout << "╠══════════════════════════════════════════════════════════════════╣\n";
        std::cout << "║ SIMULATION (Code Units - Dimensionless):                        ║\n";
        std::cout << "║   Length:          " << std::setw(44) << code.length << "║\n";
        std::cout << "║   Mass:            " << std::setw(44) << code.mass << "║\n";
        std::cout << "║   G:               " << std::setw(44) << code.G << "║\n";
        std::cout << "║                                                                  ║\n";
        std::cout << "║   All positions, velocities, densities, etc. in simulation      ║\n";
        std::cout << "║   are normalized to O(1) for numerical stability.               ║\n";
        std::cout << "╠══════════════════════════════════════════════════════════════════╣\n";
        std::cout << "║ OUTPUT (User-Specified - can differ from input!):                ║\n";
        std::cout << "║   Unit system:     " << std::setw(44) << std::left << units.output_name << "║\n";
        std::cout << "║   Length unit:     " << std::setw(44) << std::left << units.output.length_unit << "║\n";
        std::cout << "║   Mass unit:       " << std::setw(44) << std::left << units.output.mass_unit << "║\n";
        std::cout << "║   Time unit:       " << std::setw(44) << std::left << units.output.time_unit << "║\n";
        std::cout << "║   Density unit:    " << std::setw(44) << std::left << units.output.density_unit << "║\n";
        std::cout << "╠══════════════════════════════════════════════════════════════════╣\n";
        std::cout << "║ CHARACTERISTIC SCALES (Internal - SI):                           ║\n";
        std::cout << "║   [L] = " << std::scientific << std::setprecision(4) 
                  << std::setw(12) << units.length_SI << " m" << std::setw(35) << " " << "║\n";
        std::cout << "║   [M] = " << std::setw(12) << units.mass_SI << " kg" << std::setw(34) << " " << "║\n";
        std::cout << "║   [T] = " << std::setw(12) << units.time_SI << " s" << std::setw(35) << " " << "║\n";
        std::cout << "║   [V] = " << std::setw(12) << units.velocity_SI << " m/s" << std::setw(33) << " " << "║\n";
        std::cout << "║   [ρ] = " << std::setw(12) << units.density_SI << " kg/m³" << std::setw(31) << " " << "║\n";
        std::cout << "╚══════════════════════════════════════════════════════════════════╝\n";
        std::cout << std::fixed;  // Reset formatting
    }
    
    /**
     * @brief Compact print for quick reference
     */
    void print_compact() const {
        std::cout << "Unit System: " << units.input_name << " → Code (dimensionless) → " 
                  << units.output_name << "\n";
        std::cout << "  Char. scales: L=" << input.length << " " << units.input.length_unit 
                  << ", M=" << input.mass << " " << units.input.mass_unit << "\n";
    }
};

}  // namespace sph

#endif  // FLEXIBLE_UNIT_PARAMS_HPP
