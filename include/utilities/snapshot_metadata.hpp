#pragma once

#include <string>
#include <vector>
#include <map>
#include "utilities/unit_system.hpp"

namespace sph
{
    /**
     * @brief Column metadata for CSV output files
     * 
     * Describes a single column in the CSV file including its name,
     * physical unit, and description.
     */
    struct ColumnMetadata
    {
        std::string name;        // Column name (e.g., "time", "pos_x", "dens")
        std::string unit;        // Physical unit (e.g., "s", "m", "kg/m^3")
        std::string description; // Human-readable description
        
        ColumnMetadata() = default;
        ColumnMetadata(const std::string& n, const std::string& u, const std::string& d)
            : name(n), unit(u), description(d) {}
    };

    /**
     * @brief Snapshot metadata for CSV output files
     * 
     * Contains all metadata for a snapshot including simulation state,
     * unit system, and column descriptions. This is serialized to JSON
     * as a sidecar file alongside the CSV output.
     */
    struct SnapshotMetadata
    {
        // Format version for backward compatibility
        std::string format_version = "1.0";
        
        // Simulation state
        struct SimulationInfo
        {
            double time = 0.0;
            int snapshot_number = 0;
            int dimension = 0;
            int particle_count = 0;
        } simulation;
        
        // Unit system
        struct Units
        {
            std::string length;
            std::string time;
            std::string mass;
            std::string density;
            std::string pressure;
            std::string energy;
        } units;
        
        // Conversion factors from internal (code) units to output units
        struct ConversionFactors
        {
            double length_factor = 1.0;
            double time_factor = 1.0;
            double mass_factor = 1.0;
            double density_factor = 1.0;
            double pressure_factor = 1.0;
            double energy_factor = 1.0;
        } conversion_factors;
        
        // Column descriptions
        std::vector<ColumnMetadata> columns;
        
        /**
         * @brief Construct metadata from unit system
         */
        void from_unit_system(const UnitSystem& unit_sys);
        
        /**
         * @brief Serialize to JSON string
         */
        std::string to_json() const;
        
        /**
         * @brief Parse from JSON string
         */
        static SnapshotMetadata from_json(const std::string& json_str);
        
        /**
         * @brief Write metadata to file
         */
        void write_to_file(const std::string& filepath) const;
        
        /**
         * @brief Read metadata from file
         */
        static SnapshotMetadata read_from_file(const std::string& filepath);
    };

} // namespace sph
