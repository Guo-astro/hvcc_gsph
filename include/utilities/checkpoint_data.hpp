#ifndef SPH_CHECKPOINT_DATA_HPP
#define SPH_CHECKPOINT_DATA_HPP

#include <vector>
#include <string>
#include <memory>
#include <cstdint>
#include "core/particle.hpp"
#include "core/parameters.hpp"

namespace sph {

/**
 * @brief Container for checkpoint data
 * 
 * Binary Format Specification:
 * ============================
 * 
 * [HEADER: 512 bytes fixed size]
 * --------------------------------
 * Offset | Size | Field | Description
 * -------|------|-------|-------------
 * 0      | 8    | magic | "SPHCHKPT" (magic number for file type identification)
 * 8      | 4    | version | uint32_t format version (current: 1)
 * 12     | 4    | dimension | int32_t (1, 2, or 3)
 * 16     | 64   | timestamp | ISO 8601 string (YYYY-MM-DDTHH:MM:SS.sssZ)
 * 80     | 128  | simulation_name | null-terminated string
 * 208    | 64   | sph_type | null-terminated string ("GSPH", "DISPH", "GDISPH")
 * 272    | 8    | time | double (simulation time)
 * 280    | 8    | dt | double (current timestep)
 * 288    | 8    | step | int64_t (timestep number)
 * 296    | 8    | particle_count | int64_t (number of particles)
 * 304    | 8    | params_size | int64_t (size of JSON parameters section)
 * 312    | 200  | reserved | Reserved for future use (zeros)
 * 
 * [PARAMETERS: variable length]
 * --------------------------------
 * JSON-encoded SPHParameters object
 * Size determined by params_size field in header
 * 
 * [PARTICLE DATA: N × sizeof(SPHParticle)]
 * --------------------------------
 * Binary dump of particle array
 * Each particle is serialized as a fixed-size struct
 * Size = particle_count × sizeof(SPHParticle)
 * 
 * [CHECKSUM: 32 bytes]
 * --------------------------------
 * SHA-256 hash of all preceding data (header + params + particles)
 * Used for data integrity verification
 * 
 * Total File Size:
 * 512 (header) + params_size + (particle_count × sizeof(SPHParticle)) + 32 (checksum)
 * 
 * Example for 10,000 particles:
 * ~512 bytes + ~2 KB + (10,000 × ~200 bytes) + 32 bytes ≈ 2 MB
 */
struct CheckpointData {
    // Simulation state
    real time;          ///< Current simulation time
    real dt;            ///< Current timestep size
    int64_t step;       ///< Current timestep number
    
    // Particle data
    std::vector<SPHParticle> particles;  ///< All particle data
    
    // Parameters (subset needed for resume)
    std::shared_ptr<SPHParameters> params;  ///< Simulation parameters
    
    // Metadata
    std::string simulation_name;  ///< Name of simulation (e.g., "sedov_taylor_2d")
    std::string sph_type;         ///< SPH method: "GSPH", "DISPH", "GDISPH"
    int dimension;                ///< Spatial dimension: 1, 2, or 3
    std::string created_at;       ///< ISO 8601 timestamp of checkpoint creation
    
    // Version for format compatibility
    static constexpr uint32_t FORMAT_VERSION = 1;
    static constexpr char MAGIC[9] = "SPHCHKPT";  ///< File magic number
    
    // Header size constant
    static constexpr size_t HEADER_SIZE = 512;
    static constexpr size_t CHECKSUM_SIZE = 32;  // SHA-256
    
    /**
     * @brief Validate checkpoint data integrity and consistency
     * @return true if checkpoint data is valid
     */
    bool is_valid() const;
    
    /**
     * @brief Get human-readable summary of checkpoint contents
     * @return Multi-line string with checkpoint information
     */
    std::string get_info() const;
    
    /**
     * @brief Get total size in bytes (header + params + particles + checksum)
     * @return Estimated file size in bytes
     */
    size_t get_total_size() const;
};

/**
 * @brief Validation result for checkpoint files
 */
struct CheckpointValidation {
    bool is_valid;                      ///< Overall validation status
    std::string error_message;          ///< Error description if invalid
    std::vector<std::string> warnings;  ///< Non-fatal warnings
    
    // Detailed checks
    bool magic_ok;          ///< Magic number matches
    bool version_ok;        ///< Format version supported
    bool checksum_ok;       ///< Data checksum valid
    bool params_ok;         ///< Parameters valid
    bool particles_ok;      ///< Particle data valid
    
    /**
     * @brief Create a validation result for a valid checkpoint
     */
    static CheckpointValidation valid() {
        return {true, "", {}, true, true, true, true, true};
    }
    
    /**
     * @brief Create a validation result for an invalid checkpoint
     * @param error Error message describing the problem
     */
    static CheckpointValidation invalid(const std::string& error) {
        return {false, error, {}, false, false, false, false, false};
    }
};

} // namespace sph

#endif // SPH_CHECKPOINT_DATA_HPP
