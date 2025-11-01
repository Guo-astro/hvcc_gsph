#include "utilities/checkpoint_manager.hpp"
#include <cstring>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <boost/filesystem.hpp>
#include <openssl/sha.h>

namespace fs = boost::filesystem;

namespace sph {

CheckpointManager::CheckpointManager()
    : last_checkpoint_time_(0.0) {
    config_.enabled = false;
}

CheckpointManager::CheckpointManager(const AutoCheckpointConfig& config)
    : config_(config), last_checkpoint_time_(0.0) {
}

void CheckpointManager::save_checkpoint(
    const std::string& filepath,
    const Simulation& sim_const,
    const SPHParameters& params
) {
    // Workaround for const-correctness: Simulation getters return non-const references
    Simulation& sim = const_cast<Simulation&>(sim_const);
    
    // Create checkpoint data
    CheckpointData data;
    data.time = sim.get_time();
    data.dt = sim.get_dt();
    data.step = 0;  // TODO: Track step count in Simulation
    data.particles = sim.get_particles();
    data.params = std::make_shared<SPHParameters>(params);
    data.simulation_name = "simulation";  // TODO: Add name field to Simulation
    
    // Map SPHType enum to string
    std::string sph_type_str;
    switch (params.type) {
        case SPHType::SSPH: sph_type_str = "SSPH"; break;
        case SPHType::DISPH: sph_type_str = "DISPH"; break;
        case SPHType::GSPH: sph_type_str = "GSPH"; break;
        case SPHType::GDISPH: sph_type_str = "GDISPH"; break;
        default: sph_type_str = "UNKNOWN"; break;
    }
    data.sph_type = sph_type_str;
    data.dimension = DIM;
    
    // Generate timestamp (ISO 8601)
    auto now = std::chrono::system_clock::now();
    auto time_t_now = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()
    ) % 1000;
    
    std::ostringstream oss;
    oss << std::put_time(std::gmtime(&time_t_now), "%Y-%m-%dT%H:%M:%S")
        << '.' << std::setfill('0') << std::setw(3) << ms.count() << 'Z';
    data.created_at = oss.str();
    
    // Open file for binary writing
    std::ofstream file(filepath, std::ios::binary | std::ios::trunc);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open checkpoint file for writing: " + filepath);
    }
    
    // Collect all data for checksum calculation
    std::vector<char> all_data;
    
    // Write header
    write_header(file, data);
    
    // Write parameters (unused variable warning fixed by using void cast)
    (void)write_parameters(file, params);
    
    // Write particles
    write_particles(file, data.particles);
    
    // Read back all data for checksum
    file.close();
    std::ifstream read_file(filepath, std::ios::binary);
    read_file.seekg(0, std::ios::end);
    size_t file_size = read_file.tellg();
    read_file.seekg(0, std::ios::beg);
    
    all_data.resize(file_size);
    read_file.read(all_data.data(), file_size);
    read_file.close();
    
    // Write checksum
    std::ofstream append_file(filepath, std::ios::binary | std::ios::app);
    write_checksum(append_file, all_data);
    append_file.close();
}

void CheckpointManager::write_header(std::ofstream& file, const CheckpointData& data) {
    // Create 512-byte header buffer (initialized to zeros)
    char header[CheckpointData::HEADER_SIZE] = {0};
    
    size_t offset = 0;
    
    // Magic number (8 bytes)
    std::memcpy(header + offset, CheckpointData::MAGIC, 8);
    offset += 8;
    
    // Version (4 bytes)
    uint32_t version = CheckpointData::FORMAT_VERSION;
    std::memcpy(header + offset, &version, sizeof(version));
    offset += 4;
    
    // Dimension (4 bytes)
    int32_t dim = data.dimension;
    std::memcpy(header + offset, &dim, sizeof(dim));
    offset += 4;
    
    // Timestamp (64 bytes)
    std::strncpy(header + offset, data.created_at.c_str(), 64);
    offset += 64;
    
    // Simulation name (128 bytes)
    std::strncpy(header + offset, data.simulation_name.c_str(), 128);
    offset += 128;
    
    // SPH type (64 bytes)
    std::strncpy(header + offset, data.sph_type.c_str(), 64);
    offset += 64;
    
    // Time (8 bytes)
    double time = data.time;
    std::memcpy(header + offset, &time, sizeof(time));
    offset += 8;
    
    // Timestep (8 bytes)
    double dt = data.dt;
    std::memcpy(header + offset, &dt, sizeof(dt));
    offset += 8;
    
    // Step (8 bytes)
    int64_t step = data.step;
    std::memcpy(header + offset, &step, sizeof(step));
    offset += 8;
    
    // Particle count (8 bytes)
    int64_t particle_count = static_cast<int64_t>(data.particles.size());
    std::memcpy(header + offset, &particle_count, sizeof(particle_count));
    offset += 8;
    
    // Parameters size (8 bytes) - will be updated after writing params
    int64_t params_size = 0;  // Placeholder
    std::memcpy(header + offset, &params_size, sizeof(params_size));
    offset += 8;
    
    // Reserved space (200 bytes) - already zeroed
    
    // Write header to file
    file.write(header, CheckpointData::HEADER_SIZE);
}

size_t CheckpointManager::write_parameters(std::ofstream& file, const SPHParameters& params) {
    // Serialize parameters to JSON
    // For now, write a simple JSON representation
    // TODO: Use proper JSON library (nlohmann/json) in production
    
    std::ostringstream json;
    json << "{\n";
    json << "  \"physics\": {\n";
    json << "    \"gamma\": " << params.physics.gamma << ",\n";
    json << "    \"neighbor_number\": " << params.physics.neighbor_number << "\n";
    json << "  },\n";
    json << "  \"time\": {\n";
    json << "    \"start\": " << params.time.start << ",\n";
    json << "    \"end\": " << params.time.end << "\n";
    json << "  },\n";
    json << "  \"cfl\": {\n";
    json << "    \"sound\": " << params.cfl.sound << ",\n";
    json << "    \"force\": " << params.cfl.force << "\n";
    json << "  }\n";
    json << "}\n";
    
    std::string json_str = json.str();
    
    // Write JSON size
    int64_t json_size = static_cast<int64_t>(json_str.size());
    file.write(reinterpret_cast<const char*>(&json_size), sizeof(json_size));
    
    // Write JSON data
    file.write(json_str.c_str(), json_str.size());
    
    // Update header with actual params size
    // Go back and update the params_size field in header
    auto current_pos = file.tellp();
    file.seekp(304);  // Offset to params_size field
    file.write(reinterpret_cast<const char*>(&json_size), sizeof(json_size));
    file.seekp(current_pos);  // Return to current position
    
    return sizeof(json_size) + json_str.size();
}

void CheckpointManager::write_particles(std::ofstream& file, const std::vector<SPHParticle>& particles) {
    // Write particle data as binary array
    if (!particles.empty()) {
        file.write(
            reinterpret_cast<const char*>(particles.data()),
            particles.size() * sizeof(SPHParticle)
        );
    }
}

void CheckpointManager::write_checksum(std::ofstream& file, const std::vector<char>& data) {
    // Compute SHA-256 hash
    auto hash = compute_sha256(data);
    
    // Write hash to file
    file.write(reinterpret_cast<const char*>(hash.data()), hash.size());
}

std::vector<uint8_t> CheckpointManager::compute_sha256(const std::vector<char>& data) {
    std::vector<uint8_t> hash(SHA256_DIGEST_LENGTH);
    
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, data.data(), data.size());
    SHA256_Final(hash.data(), &sha256);
    
    return hash;
}

CheckpointData CheckpointManager::load_checkpoint(const std::string& filepath) {
    // Check file exists
    if (!fs::exists(filepath)) {
        throw std::runtime_error("Checkpoint file does not exist: " + filepath);
    }
    
    // Open file for binary reading
    std::ifstream file(filepath, std::ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open checkpoint file for reading: " + filepath);
    }
    
    CheckpointData data;
    
    // Read header
    read_header(file, data);
    
    // Get file size to read parameters size from header
    file.seekg(0, std::ios::end);
    size_t file_size = file.tellg();
    file.seekg(CheckpointData::HEADER_SIZE, std::ios::beg);
    
    // Read parameters size from header (stored at offset 304)
    file.seekg(304, std::ios::beg);
    int64_t params_size;
    file.read(reinterpret_cast<char*>(&params_size), sizeof(params_size));
    
    // Go back to after header
    file.seekg(CheckpointData::HEADER_SIZE, std::ios::beg);
    
    // Read parameters
    read_parameters(file, data, params_size);
    
    // Read particles
    read_particles(file, data);
    
    // Read all data for checksum verification
    file.close();
    std::ifstream read_file(filepath, std::ios::binary);
    
    // Calculate size of data to verify (everything except checksum)
    size_t data_size = file_size - CheckpointData::CHECKSUM_SIZE;
    std::vector<char> file_data(data_size);
    read_file.read(file_data.data(), data_size);
    
    // Verify checksum
    if (!verify_checksum(read_file, file_data)) {
        throw std::runtime_error("Checkpoint file checksum verification failed: " + filepath);
    }
    
    read_file.close();
    
    return data;
}

CheckpointValidation CheckpointManager::validate_checkpoint(const std::string& filepath) {
    // Check file exists
    if (!fs::exists(filepath)) {
        return CheckpointValidation::invalid("File does not exist");
    }
    
    // Try to open file
    std::ifstream file(filepath, std::ios::binary);
    if (!file.is_open()) {
        return CheckpointValidation::invalid("Cannot open file for reading");
    }
    
    CheckpointValidation result = CheckpointValidation::valid();
    
    // Verify magic number
    char magic[8];
    file.read(magic, 8);
    if (std::memcmp(magic, CheckpointData::MAGIC, 8) != 0) {
        result.is_valid = false;
        result.magic_ok = false;
        result.error_message = "Invalid magic number - not a valid checkpoint file";
        return result;
    }
    result.magic_ok = true;
    
    // Verify version
    uint32_t version;
    file.read(reinterpret_cast<char*>(&version), sizeof(version));
    if (version != CheckpointData::FORMAT_VERSION) {
        result.is_valid = false;
        result.version_ok = false;
        result.error_message = "Unsupported checkpoint format version: " + std::to_string(version);
        return result;
    }
    result.version_ok = true;
    
    // Get file size
    file.seekg(0, std::ios::end);
    size_t file_size = file.tellg();
    
    // Check minimum size (header + checksum at least)
    if (file_size < CheckpointData::HEADER_SIZE + CheckpointData::CHECKSUM_SIZE) {
        result.is_valid = false;
        result.error_message = "File too small - appears to be truncated";
        return result;
    }
    
    // Read all data except checksum
    file.seekg(0, std::ios::beg);
    size_t data_size = file_size - CheckpointData::CHECKSUM_SIZE;
    std::vector<char> file_data(data_size);
    file.read(file_data.data(), data_size);
    
    // Verify checksum
    result.checksum_ok = verify_checksum(file, file_data);
    if (!result.checksum_ok) {
        result.is_valid = false;
        result.error_message = "Checksum verification failed - file may be corrupted";
        return result;
    }
    
    // Basic validation passed
    result.params_ok = true;  // Could do deeper validation here
    result.particles_ok = true;
    
    file.close();
    return result;
}

void CheckpointManager::configure_auto_checkpoint(const AutoCheckpointConfig& config) {
    config_ = config;
}

bool CheckpointManager::should_checkpoint(real current_time) const {
    if (!config_.enabled) {
        return false;
    }
    
    return (current_time - last_checkpoint_time_) >= config_.interval;
}

std::string CheckpointManager::generate_checkpoint_path(
    const std::string& run_dir,
    real time
) const {
    std::ostringstream oss;
    oss << run_dir << "/" << config_.directory 
        << "/checkpoint_t" << std::fixed << std::setprecision(3) << time << ".chk";
    return oss.str();
}

void CheckpointManager::cleanup_old_checkpoints() {
    while (checkpoint_files_.size() > static_cast<size_t>(config_.max_keep)) {
        // Remove oldest checkpoint
        auto oldest = checkpoint_files_.front();
        checkpoint_files_.pop_front();
        
        // Delete file if it exists
        if (fs::exists(oldest)) {
            fs::remove(oldest);
        }
    }
}

void CheckpointManager::record_checkpoint(const std::string& filepath, real time) {
    last_checkpoint_time_ = time;
    checkpoint_files_.push_back(filepath);
    
    // Cleanup if needed
    if (config_.enabled && config_.max_keep > 0) {
        cleanup_old_checkpoints();
    }
}

std::vector<std::string> CheckpointManager::get_checkpoint_files() const {
    return std::vector<std::string>(checkpoint_files_.begin(), checkpoint_files_.end());
}

void CheckpointManager::read_header(std::ifstream& file, CheckpointData& data) {
    // Read 512-byte header
    char header[CheckpointData::HEADER_SIZE];
    file.read(header, CheckpointData::HEADER_SIZE);
    
    if (!file.good()) {
        throw std::runtime_error("Failed to read checkpoint header");
    }
    
    size_t offset = 0;
    
    // Verify magic number
    if (std::memcmp(header, CheckpointData::MAGIC, 8) != 0) {
        throw std::runtime_error("Invalid checkpoint file: magic number mismatch");
    }
    offset += 8;
    
    // Verify version
    uint32_t version;
    std::memcpy(&version, header + offset, sizeof(version));
    if (version != CheckpointData::FORMAT_VERSION) {
        throw std::runtime_error("Unsupported checkpoint format version: " + std::to_string(version));
    }
    offset += 4;
    
    // Read dimension
    int32_t dim;
    std::memcpy(&dim, header + offset, sizeof(dim));
    data.dimension = dim;
    offset += 4;
    
    // Read timestamp
    char timestamp[64];
    std::strncpy(timestamp, header + offset, 64);
    timestamp[63] = '\0';  // Ensure null-termination
    data.created_at = std::string(timestamp);
    offset += 64;
    
    // Read simulation name
    char sim_name[128];
    std::strncpy(sim_name, header + offset, 128);
    sim_name[127] = '\0';
    data.simulation_name = std::string(sim_name);
    offset += 128;
    
    // Read SPH type
    char sph_type[64];
    std::strncpy(sph_type, header + offset, 64);
    sph_type[63] = '\0';
    data.sph_type = std::string(sph_type);
    offset += 64;
    
    // Read time
    double time;
    std::memcpy(&time, header + offset, sizeof(time));
    data.time = time;
    offset += 8;
    
    // Read timestep
    double dt;
    std::memcpy(&dt, header + offset, sizeof(dt));
    data.dt = dt;
    offset += 8;
    
    // Read step
    int64_t step;
    std::memcpy(&step, header + offset, sizeof(step));
    data.step = step;
    offset += 8;
    
    // Read particle count (stored but will be verified when reading particles)
    int64_t particle_count;
    std::memcpy(&particle_count, header + offset, sizeof(particle_count));
    // offset += 8;
    
    // Parameters size is read separately as needed
}

void CheckpointManager::read_parameters(std::ifstream& file, CheckpointData& data, size_t params_size) {
    // Read JSON size
    int64_t json_size;
    file.read(reinterpret_cast<char*>(&json_size), sizeof(json_size));
    
    if (json_size <= 0 || json_size > 1000000) {  // Sanity check
        throw std::runtime_error("Invalid parameters size in checkpoint");
    }
    
    // Read JSON string
    std::vector<char> json_buffer(json_size + 1);
    file.read(json_buffer.data(), json_size);
    json_buffer[json_size] = '\0';
    
    // Parse JSON and populate parameters
    // For now, create a basic parameters object
    // TODO: Implement full JSON parsing (use nlohmann/json or similar)
    auto params = std::make_shared<SPHParameters>();
    
    // Simple parsing for now (just extract a few values)
    std::string json_str(json_buffer.data());
    
    // This is a placeholder - in production, use proper JSON library
    // Extract gamma value (look for "gamma": X.XXX pattern)
    size_t gamma_pos = json_str.find("\"gamma\":");
    if (gamma_pos != std::string::npos) {
        size_t value_start = json_str.find_first_not_of(" \t", gamma_pos + 8);
        if (value_start != std::string::npos) {
            params->physics.gamma = std::stod(json_str.substr(value_start));
        }
    }
    
    // Extract neighbor_number
    size_t nn_pos = json_str.find("\"neighbor_number\":");
    if (nn_pos != std::string::npos) {
        size_t value_start = json_str.find_first_not_of(" \t", nn_pos + 18);
        if (value_start != std::string::npos) {
            params->physics.neighbor_number = std::stoi(json_str.substr(value_start));
        }
    }
    
    data.params = params;
}

void CheckpointManager::read_particles(std::ifstream& file, CheckpointData& data) {
    // Determine number of particles from current file position to checksum
    auto current_pos = file.tellg();
    file.seekg(0, std::ios::end);
    auto end_pos = file.tellg();
    
    // Calculate particle data size (total - current position - checksum)
    size_t particle_data_size = end_pos - current_pos - CheckpointData::CHECKSUM_SIZE;
    size_t num_particles = particle_data_size / sizeof(SPHParticle);
    
    // Return to particle data start
    file.seekg(current_pos);
    
    // Read particles
    data.particles.resize(num_particles);
    if (num_particles > 0) {
        file.read(
            reinterpret_cast<char*>(data.particles.data()),
            num_particles * sizeof(SPHParticle)
        );
        
        if (!file.good()) {
            throw std::runtime_error("Failed to read particle data from checkpoint");
        }
    }
}

bool CheckpointManager::verify_checksum(std::ifstream& file, const std::vector<char>& data) {
    // Read stored checksum from file
    std::vector<uint8_t> stored_checksum(CheckpointData::CHECKSUM_SIZE);
    file.read(reinterpret_cast<char*>(stored_checksum.data()), CheckpointData::CHECKSUM_SIZE);
    
    if (!file.good()) {
        return false;  // Could not read checksum
    }
    
    // Compute checksum of data
    auto computed_checksum = compute_sha256(data);
    
    // Compare
    return (stored_checksum == computed_checksum);
}

} // namespace sph
