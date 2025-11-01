#pragma once
#include "utilities/unit_system.hpp"
#include <cmath>

namespace sph
{
    /**
     * @brief Physical constants for unit conversions
     */
    namespace PhysicalConstants
    {
        // Fundamental constants (SI units)
        constexpr double G_SI = 6.67430e-11;        // m^3 kg^-1 s^-2
        constexpr double c_SI = 2.99792458e8;       // m/s
        constexpr double M_sun_SI = 1.98847e30;     // kg
        constexpr double pc_SI = 3.0856775814913673e16; // m (parsec)
        constexpr double kpc_SI = pc_SI * 1e3;      // m (kiloparsec)
        constexpr double yr_SI = 3.15576e7;         // s (year)
        constexpr double Myr_SI = yr_SI * 1e6;      // s (megayear)
        constexpr double Gyr_SI = yr_SI * 1e9;      // s (gigayear)
        constexpr double km_SI = 1e3;               // m (kilometer)
        
        // CGS conversions
        constexpr double cm_to_m = 1e-2;
        constexpr double g_to_kg = 1e-3;
        constexpr double dyne_to_N = 1e-5;
        constexpr double erg_to_J = 1e-7;
    }

    /**
     * @brief Enumeration of supported unit systems
     */
    enum class UnitSystemType
    {
        DIMENSIONLESS,  // No units (all values = 1)
        SI,             // International System (m, kg, s)
        CGS,            // Centimeter-Gram-Second
        GALACTIC_KPC,   // Astrophysical (kpc, M_sun, km/s, Myr)
        GALACTIC_PC,    // Astrophysical (pc, M_sun, km/s, Myr)
        CUSTOM          // User-defined
    };

    /**
     * @brief Factory class for creating unit systems
     */
    class UnitFactory
    {
    public:
        /**
         * @brief Create a unit system based on type
         * @param type The type of unit system to create
         * @return UnitSystem with appropriate conversion factors
         */
        static UnitSystem create(UnitSystemType type)
        {
            switch (type)
            {
                case UnitSystemType::DIMENSIONLESS:
                    return createDimensionless();
                case UnitSystemType::SI:
                    return createSI();
                case UnitSystemType::CGS:
                    return createCGS();
                case UnitSystemType::GALACTIC_KPC:
                    return createGalacticKpc();
                case UnitSystemType::GALACTIC_PC:
                    return createGalacticPc();
                default:
                    return createSI();
            }
        }

        /**
         * @brief Dimensionless units (code units)
         */
        static UnitSystem createDimensionless()
        {
            UnitSystem units;
            units.time_factor = 1.0;
            units.length_factor = 1.0;
            units.mass_factor = 1.0;
            units.density_factor = 1.0;
            units.pressure_factor = 1.0;
            units.energy_factor = 1.0;
            
            units.time_unit = "";
            units.length_unit = "";
            units.mass_unit = "";
            units.density_unit = "";
            units.pressure_unit = "";
            units.energy_unit = "";
            
            return units;
        }

        /**
         * @brief SI units (meters, kilograms, seconds)
         */
        static UnitSystem createSI()
        {
            UnitSystem units;
            units.time_factor = 1.0;
            units.length_factor = 1.0;
            units.mass_factor = 1.0;
            units.density_factor = 1.0;
            units.pressure_factor = 1.0;
            units.energy_factor = 1.0;
            
            units.time_unit = "s";
            units.length_unit = "m";
            units.mass_unit = "kg";
            units.density_unit = "kg/m^3";
            units.pressure_unit = "Pa";
            units.energy_unit = "J/kg";
            
            return units;
        }

        /**
         * @brief CGS units (centimeters, grams, seconds)
         */
        static UnitSystem createCGS()
        {
            using namespace PhysicalConstants;
            
            UnitSystem units;
            units.time_factor = 1.0;                    // s
            units.length_factor = 1.0 / cm_to_m;        // m -> cm
            units.mass_factor = 1.0 / g_to_kg;          // kg -> g
            units.density_factor = (1.0 / g_to_kg) / std::pow(1.0 / cm_to_m, 3); // kg/m^3 -> g/cm^3
            units.pressure_factor = dyne_to_N / std::pow(cm_to_m, 2); // Pa -> dyne/cm^2
            units.energy_factor = erg_to_J / g_to_kg;   // J/kg -> erg/g
            
            units.time_unit = "s";
            units.length_unit = "cm";
            units.mass_unit = "g";
            units.density_unit = "g/cm^3";
            units.pressure_unit = "dyne/cm^2";
            units.energy_unit = "erg/g";
            
            return units;
        }

        /**
         * @brief Galactic units with kpc (kiloparsecs, solar masses, km/s, Myr)
         * Commonly used for galaxy simulations
         */
        static UnitSystem createGalacticKpc()
        {
            using namespace PhysicalConstants;
            
            UnitSystem units;
            
            // Base units: kpc, M_sun, Myr
            const double length_unit = kpc_SI;      // 1 kpc in meters
            const double mass_unit = M_sun_SI;      // 1 M_sun in kg
            const double time_unit = Myr_SI;        // 1 Myr in seconds
            
            units.time_factor = 1.0 / time_unit;
            units.length_factor = 1.0 / length_unit;
            units.mass_factor = 1.0 / mass_unit;
            
            // Derived units
            // Velocity: kpc/Myr = (kpc * 1e3 m/kpc) / (Myr * 1e6 yr/Myr * 3.15576e7 s/yr)
            //         ≈ 0.9778 km/s
            // For convenience, we often express velocity in km/s
            const double velocity_unit = length_unit / time_unit; // m/s
            const double velocity_factor_kms = velocity_unit / km_SI;
            
            // Density: M_sun/kpc^3
            units.density_factor = 1.0 / (mass_unit / std::pow(length_unit, 3));
            
            // Pressure: M_sun/(kpc·Myr^2)
            units.pressure_factor = 1.0 / (mass_unit / (length_unit * std::pow(time_unit, 2)));
            
            // Specific energy: (kpc/Myr)^2 = kpc^2/Myr^2
            units.energy_factor = 1.0 / std::pow(velocity_unit, 2);
            
            units.time_unit = "Myr";
            units.length_unit = "kpc";
            units.mass_unit = "M_sun";
            units.density_unit = "M_sun/kpc^3";
            units.pressure_unit = "M_sun/(kpc*Myr^2)";
            units.energy_unit = "(kpc/Myr)^2";
            
            return units;
        }

        /**
         * @brief Galactic units with pc (parsecs, solar masses, km/s, Myr)
         * Used for smaller-scale astrophysical simulations
         */
        static UnitSystem createGalacticPc()
        {
            using namespace PhysicalConstants;
            
            UnitSystem units;
            
            // Base units: pc, M_sun, Myr
            const double length_unit = pc_SI;       // 1 pc in meters
            const double mass_unit = M_sun_SI;      // 1 M_sun in kg
            const double time_unit = Myr_SI;        // 1 Myr in seconds
            
            units.time_factor = 1.0 / time_unit;
            units.length_factor = 1.0 / length_unit;
            units.mass_factor = 1.0 / mass_unit;
            
            // Derived units
            units.density_factor = 1.0 / (mass_unit / std::pow(length_unit, 3));
            units.pressure_factor = 1.0 / (mass_unit / (length_unit * std::pow(time_unit, 2)));
            units.energy_factor = 1.0 / std::pow(length_unit / time_unit, 2);
            
            units.time_unit = "Myr";
            units.length_unit = "pc";
            units.mass_unit = "M_sun";
            units.density_unit = "M_sun/pc^3";
            units.pressure_unit = "M_sun/(pc*Myr^2)";
            units.energy_unit = "(pc/Myr)^2";
            
            return units;
        }

        /**
         * @brief Create custom unit system
         * @param length_in_m Length unit in meters (e.g., 1e3 for km)
         * @param mass_in_kg Mass unit in kilograms
         * @param time_in_s Time unit in seconds
         * @param length_label Label for length unit
         * @param mass_label Label for mass unit
         * @param time_label Label for time unit
         */
        static UnitSystem createCustom(
            double length_in_m, double mass_in_kg, double time_in_s,
            const std::string& length_label, const std::string& mass_label, 
            const std::string& time_label)
        {
            UnitSystem units;
            
            units.time_factor = 1.0 / time_in_s;
            units.length_factor = 1.0 / length_in_m;
            units.mass_factor = 1.0 / mass_in_kg;
            
            // Derived units
            units.density_factor = 1.0 / (mass_in_kg / std::pow(length_in_m, 3));
            units.pressure_factor = 1.0 / (mass_in_kg / (length_in_m * std::pow(time_in_s, 2)));
            units.energy_factor = 1.0 / std::pow(length_in_m / time_in_s, 2);
            
            units.time_unit = time_label;
            units.length_unit = length_label;
            units.mass_unit = mass_label;
            units.density_unit = mass_label + "/" + length_label + "^3";
            units.pressure_unit = mass_label + "/(" + length_label + "*" + time_label + "^2)";
            units.energy_unit = "(" + length_label + "/" + time_label + ")^2";
            
            return units;
        }

        /**
         * @brief Get unit system name
         */
        static std::string getTypeName(UnitSystemType type)
        {
            switch (type)
            {
                case UnitSystemType::DIMENSIONLESS: return "Dimensionless";
                case UnitSystemType::SI: return "SI";
                case UnitSystemType::CGS: return "CGS";
                case UnitSystemType::GALACTIC_KPC: return "Galactic (kpc)";
                case UnitSystemType::GALACTIC_PC: return "Galactic (pc)";
                case UnitSystemType::CUSTOM: return "Custom";
                default: return "Unknown";
            }
        }
    };

} // namespace sph
