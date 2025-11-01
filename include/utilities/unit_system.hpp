#pragma once
#include <string>

namespace sph
{

    struct UnitSystem
    {
        // Conversion factors: multiply your internal (SI) values by these factors
        // to convert them into the desired unit system.
        double time_factor;     // e.g., 1.0 for seconds
        double length_factor;   // e.g., 1.0 for meters, 100.0 for centimeters, etc.
        double mass_factor;     // e.g., 1.0 for kg, 1e3 for grams, etc.
        double density_factor;  // e.g., 1.0 for kg/m^3, 1e-3 for g/cm^3, etc.
        double pressure_factor; // e.g., 1.0 for Pa
        double energy_factor;   // e.g., 1.0 for J/kg

        // Unit labels for output header
        std::string time_unit;
        std::string length_unit;
        std::string mass_unit;
        std::string density_unit;
        std::string pressure_unit;
        std::string energy_unit;

        // Default constructor sets SI units.
        UnitSystem()
            : time_factor(1.0), length_factor(1.0), mass_factor(1.0),
              density_factor(1.0), pressure_factor(1.0), energy_factor(1.0),
              time_unit("s"), length_unit("m"), mass_unit("kg"),
              density_unit("kg/m^3"), pressure_unit("Pa"), energy_unit("J/kg")
        {
        }
    };

} // namespace sph
