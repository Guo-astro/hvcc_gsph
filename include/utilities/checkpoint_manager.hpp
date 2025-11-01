#ifndef SPH_CHECKPOINT_MANAGER_HPP
#define SPH_CHECKPOINT_MANAGER_HPP

#include "checkpoint_data.hpp"
#include "core/simulation.hpp"
#include "core/parameters.hpp"
#include <string>
#include <deque>
#include <memory>
#include <boost/filesystem.hpp>
#include <fstream>

namespace sph {

/**
 * @brief Manages checkpoint save/load and auto-checkpointing
 * 
 * The CheckpointManager handles:
 * - Saving simulation state to checkpoint files
 * - Loading simulation state from checkpoint files
 * - Automatic checkpointing at regular intervals
 * - Cleanup of old checkpoints (keeping only recent ones)
 * - Checkpoint validation and integrity checking
 * - Signal handling for graceful interruption
 */
class CheckpointManager {
public:
    /**
     * @brief Configuration for automatic checkpointing
     */
    struct AutoCheckpointConfig {
        bool enabled = false;           ///< Enable automatic checkpointing
        real interval = 1.0;            ///< Time units between checkpoints
        int max_keep = 3;               ///< Keep last N checkpoints (delete older ones)
        bool on_interrupt = true;       ///< Save checkpoint on SIGINT/SIGTERM
        std::string directory = "checkpoints";  ///< Subdirectory for checkpoint files
        
        /**
         * @brief Create default configuration (disabled)
         */
        AutoCheckpointConfig() = default;
        
        /**
         * @brief Create enabled configuration with specified interval
         * @param interval_time Time between checkpoints
         * @param max_checkpoints Maximum number of checkpoints to keep
         */
        AutoCheckpointConfig(real interval_time, int max_checkpoints = 3)
            : enabled(true), interval(interval_time), max_keep(max_checkpoints) {}
    };
    
    /**
     * @brief Default constructor (auto-checkpoint disabled)
     */
    CheckpointManager();
    
    /**
     * @brief Constructor with auto-checkpoint configuration
     * @param config Auto-checkpoint configuration
     */
    explicit CheckpointManager(const AutoCheckpointConfig& config);
    
    /**
     * @brief Save simulation state to checkpoint file
     * 
     * Creates a binary checkpoint file containing:
     * - Header with metadata
     * - JSON-encoded parameters
     * - Binary particle data
     * - SHA-256 checksum
     * 
     * @param filepath Path to checkpoint file (will be created/overwritten)
     * @param sim Simulation object with current state
     * @param params Simulation parameters
     * @throws std::runtime_error if save fails
     */
    void save_checkpoint(
        const std::string& filepath,
        const Simulation& sim,
        const SPHParameters& params
    );
    
    /**
     * @brief Load simulation state from checkpoint file
     * 
     * Reads and validates checkpoint file, then populates CheckpointData.
     * Performs integrity checks:
     * - Magic number verification
     * - Format version compatibility
     * - Checksum validation
     * 
     * @param filepath Path to existing checkpoint file
     * @return CheckpointData with loaded simulation state
     * @throws std::runtime_error if file doesn't exist, is corrupted, or invalid
     */
    CheckpointData load_checkpoint(const std::string& filepath);
    
    /**
     * @brief Validate checkpoint file without fully loading it
     * 
     * Performs lightweight validation:
     * - File exists and is readable
     * - Magic number correct
     * - Format version supported
     * - Checksum valid
     * 
     * @param filepath Path to checkpoint file
     * @return Validation result with detailed error information
     */
    CheckpointValidation validate_checkpoint(const std::string& filepath);
    
    /**
     * @brief Configure automatic checkpointing
     * 
     * @param config Auto-checkpoint configuration
     */
    void configure_auto_checkpoint(const AutoCheckpointConfig& config);
    
    /**
     * @brief Check if a checkpoint should be saved at the current time
     * 
     * Used by simulation loop to decide when to checkpoint.
     * Returns true if:
     * - Auto-checkpoint is enabled
     * - Current time >= last_checkpoint_time + interval
     * 
     * @param current_time Current simulation time
     * @return true if checkpoint should be saved now
     */
    bool should_checkpoint(real current_time) const;
    
    /**
     * @brief Generate checkpoint file path for given run directory and time
     * 
     * Creates path following pattern:
     * {run_dir}/{checkpoint_directory}/checkpoint_t{time:.3f}.chk
     * 
     * Example: "simulations/shock_tube/run_XYZ/checkpoints/checkpoint_t0.500.chk"
     * 
     * @param run_dir Run directory path
     * @param time Simulation time for this checkpoint
     * @return Full path to checkpoint file
     */
    std::string generate_checkpoint_path(
        const std::string& run_dir,
        real time
    ) const;
    
    /**
     * @brief Clean up old checkpoint files, keeping only the most recent ones
     * 
     * Deletes oldest checkpoints until only max_keep remain.
     * Automatically called after saving a checkpoint if auto-checkpoint enabled.
     */
    void cleanup_old_checkpoints();
    
    /**
     * @brief Record that a checkpoint was saved (updates internal state)
     * 
     * Called internally after successful checkpoint save.
     * Updates last_checkpoint_time and checkpoint file list.
     * 
     * @param filepath Path to checkpoint file that was saved
     * @param time Simulation time of the checkpoint
     */
    void record_checkpoint(const std::string& filepath, real time);
    
    /**
     * @brief Get list of all checkpoint files being tracked
     * 
     * Returns paths in chronological order (oldest first).
     * Only includes checkpoints saved during current CheckpointManager lifetime.
     * 
     * @return Vector of checkpoint file paths
     */
    std::vector<std::string> get_checkpoint_files() const;
    
    /**
     * @brief Get time of last checkpoint
     * 
     * @return Simulation time of most recent checkpoint, or 0.0 if none saved
     */
    real get_last_checkpoint_time() const { return last_checkpoint_time_; }
    
    /**
     * @brief Get current auto-checkpoint configuration
     * 
     * @return Current configuration
     */
    const AutoCheckpointConfig& get_config() const { return config_; }
    
private:
    // Configuration
    AutoCheckpointConfig config_;      ///< Auto-checkpoint configuration
    real last_checkpoint_time_;        ///< Time of last checkpoint
    std::deque<std::string> checkpoint_files_;  ///< List of checkpoint files (FIFO)
    
    // Binary I/O helper methods
    
    /**
     * @brief Write checkpoint header to file
     * @param file Output file stream
     * @param data Checkpoint data
     */
    void write_header(std::ofstream& file, const CheckpointData& data);
    
    /**
     * @brief Write parameters section to file (JSON format)
     * @param file Output file stream
     * @param params Simulation parameters
     * @return Size in bytes of parameters section written
     */
    size_t write_parameters(std::ofstream& file, const SPHParameters& params);
    
    /**
     * @brief Write particle data to file (binary format)
     * @param file Output file stream
     * @param particles Vector of particles
     */
    void write_particles(std::ofstream& file, const std::vector<SPHParticle>& particles);
    
    /**
     * @brief Write checksum to file
     * @param file Output file stream
     * @param data Binary data to checksum (header + params + particles)
     */
    void write_checksum(std::ofstream& file, const std::vector<char>& data);
    
    /**
     * @brief Read checkpoint header from file
     * @param file Input file stream
     * @param data Checkpoint data to populate
     */
    void read_header(std::ifstream& file, CheckpointData& data);
    
    /**
     * @brief Read parameters section from file
     * @param file Input file stream
     * @param data Checkpoint data to populate
     * @param params_size Size of parameters section in bytes
     */
    void read_parameters(std::ifstream& file, CheckpointData& data, size_t params_size);
    
    /**
     * @brief Read particle data from file
     * @param file Input file stream
     * @param data Checkpoint data to populate
     */
    void read_particles(std::ifstream& file, CheckpointData& data);
    
    /**
     * @brief Verify checksum of loaded data
     * @param file Input file stream (positioned at checksum)
     * @param data Binary data to verify (header + params + particles)
     * @return true if checksum matches
     */
    bool verify_checksum(std::ifstream& file, const std::vector<char>& data);
    
    /**
     * @brief Compute SHA-256 hash of data
     * @param data Binary data to hash
     * @return 32-byte SHA-256 hash
     */
    std::vector<uint8_t> compute_sha256(const std::vector<char>& data);
};

} // namespace sph

#endif // SPH_CHECKPOINT_MANAGER_HPP
