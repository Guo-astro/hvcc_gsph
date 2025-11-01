#include "utilities/snapshot_metadata.hpp"
#include <fstream>
#include <sstream>
#include <iomanip>

namespace sph
{
    void SnapshotMetadata::from_unit_system(const UnitSystem& unit_sys)
    {
        units.length = unit_sys.length_unit;
        units.time = unit_sys.time_unit;
        units.mass = unit_sys.mass_unit;
        units.density = unit_sys.density_unit;
        units.pressure = unit_sys.pressure_unit;
        units.energy = unit_sys.energy_unit;
        
        conversion_factors.length_factor = unit_sys.length_factor;
        conversion_factors.time_factor = unit_sys.time_factor;
        conversion_factors.mass_factor = unit_sys.mass_factor;
        conversion_factors.density_factor = unit_sys.density_factor;
        conversion_factors.pressure_factor = unit_sys.pressure_factor;
        conversion_factors.energy_factor = unit_sys.energy_factor;
    }

    std::string SnapshotMetadata::to_json() const
    {
        std::ostringstream json;
        json << std::fixed << std::setprecision(10);
        
        json << "{\n";
        json << "  \"format_version\": \"" << format_version << "\",\n";
        
        // Simulation info
        json << "  \"simulation\": {\n";
        json << "    \"time\": " << simulation.time << ",\n";
        json << "    \"snapshot_number\": " << simulation.snapshot_number << ",\n";
        json << "    \"dimension\": " << simulation.dimension << ",\n";
        json << "    \"particle_count\": " << simulation.particle_count << "\n";
        json << "  },\n";
        
        // Units
        json << "  \"units\": {\n";
        json << "    \"length\": \"" << units.length << "\",\n";
        json << "    \"time\": \"" << units.time << "\",\n";
        json << "    \"mass\": \"" << units.mass << "\",\n";
        json << "    \"density\": \"" << units.density << "\",\n";
        json << "    \"pressure\": \"" << units.pressure << "\",\n";
        json << "    \"energy\": \"" << units.energy << "\"\n";
        json << "  },\n";
        
        // Conversion factors
        json << "  \"conversion_factors\": {\n";
        json << "    \"length_factor\": " << conversion_factors.length_factor << ",\n";
        json << "    \"time_factor\": " << conversion_factors.time_factor << ",\n";
        json << "    \"mass_factor\": " << conversion_factors.mass_factor << ",\n";
        json << "    \"density_factor\": " << conversion_factors.density_factor << ",\n";
        json << "    \"pressure_factor\": " << conversion_factors.pressure_factor << ",\n";
        json << "    \"energy_factor\": " << conversion_factors.energy_factor << "\n";
        json << "  },\n";
        
        // Columns
        json << "  \"columns\": [\n";
        for (size_t i = 0; i < columns.size(); ++i)
        {
            json << "    {\n";
            json << "      \"name\": \"" << columns[i].name << "\",\n";
            json << "      \"unit\": \"" << columns[i].unit << "\",\n";
            json << "      \"description\": \"" << columns[i].description << "\"\n";
            json << "    }";
            if (i < columns.size() - 1) json << ",";
            json << "\n";
        }
        json << "  ]\n";
        json << "}\n";
        
        return json.str();
    }

    void SnapshotMetadata::write_to_file(const std::string& filepath) const
    {
        std::ofstream out(filepath);
        if (!out.is_open())
        {
            throw std::runtime_error("Cannot open metadata file for writing: " + filepath);
        }
        out << to_json();
        out.close();
    }

    // Simplified from_json and read_from_file for now
    // Full JSON parsing would require a library like nlohmann/json or manual parsing
    SnapshotMetadata SnapshotMetadata::from_json(const std::string& json_str)
    {
        // TODO: Implement proper JSON parsing when needed
        // For now, this is just a placeholder
        SnapshotMetadata meta;
        return meta;
    }

    SnapshotMetadata SnapshotMetadata::read_from_file(const std::string& filepath)
    {
        // TODO: Implement proper JSON parsing when needed
        // For now, this is just a placeholder
        SnapshotMetadata meta;
        return meta;
    }

} // namespace sph
