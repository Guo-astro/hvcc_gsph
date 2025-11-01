#include "utilities/checkpoint_data.hpp"
#include <sstream>
#include <iomanip>

namespace sph {

// Initialize static constants
constexpr char CheckpointData::MAGIC[9];

bool CheckpointData::is_valid() const {
    // Check basic validity
    if (dimension < 1 || dimension > 3) {
        return false;
    }
    
    if (sph_type.empty() || simulation_name.empty()) {
        return false;
    }
    
    if (time < 0.0 || dt <= 0.0) {
        return false;
    }
    
    if (step < 0) {
        return false;
    }
    
    if (!params) {
        return false;
    }
    
    // Check particle data consistency
    // Note: No particle_count field in SPHParameters
    if (particles.empty()) {
        return false;
    }
    
    return true;
}

std::string CheckpointData::get_info() const {
    std::ostringstream oss;
    
    oss << "Checkpoint Information:\n";
    oss << "=====================\n";
    oss << "Simulation: " << simulation_name << "\n";
    oss << "SPH Type: " << sph_type << "\n";
    oss << "Dimension: " << dimension << "D\n";
    oss << "Time: " << time << "\n";
    oss << "Timestep: " << dt << "\n";
    oss << "Step Number: " << step << "\n";
    oss << "Particle Count: " << particles.size() << "\n";
    oss << "Created: " << created_at << "\n";
    oss << "Format Version: " << FORMAT_VERSION << "\n";
    
    if (params) {
        oss << "\nParameters:\n";
        oss << "  Gamma: " << params->physics.gamma << "\n";
        oss << "  Neighbor number: " << params->physics.neighbor_number << "\n";
        // Add more parameter info as needed
    }
    
    return oss.str();
}

size_t CheckpointData::get_total_size() const {
    size_t size = HEADER_SIZE;  // 512 bytes header
    
    // Parameters size (estimate ~2-4 KB for JSON)
    size += 3000;  // Conservative estimate
    
    // Particle data
    size += particles.size() * sizeof(SPHParticle);
    
    // Checksum
    size += CHECKSUM_SIZE;  // 32 bytes SHA-256
    
    return size;
}

} // namespace sph
