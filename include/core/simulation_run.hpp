#pragma once

#include <string>
#include <memory>
#include <vector>
#include <chrono>
#include "utilities/defines.hpp"
#include "core/output_format.hpp"

namespace sph
{
    class Simulation;
    class SPHParameters;
    class UnitSystem;
    struct SPHParticle;

    /**
     * @brief Manages a single simulation run with all associated files
     * 
     * Creates a self-contained directory structure:
     * simulations/{sample_name}/{run_id}/
     *   ├── metadata.json
     *   ├── config.json
     *   ├── initial_conditions.csv
     *   ├── outputs/
     *   │   ├── csv/
     *   │   └── binary/
     *   ├── visualizations/
     *   ├── logs/
     *   └── analysis/
     */
    class SimulationRun
    {
    public:
        struct Config
        {
            std::string base_dir = "simulations";
            std::string sample_name = "unnamed";
            std::string description = "";
            std::string sph_type = "DISPH";
            int dimension = DIM;
            
            bool auto_run_id = true;
            std::string custom_run_id = "";
            
            bool save_initial_conditions = true;
            bool save_config = true;
            bool save_metadata = true;
            bool save_source_code = true;  // NEW: Save sample source files
            bool create_latest_symlink = true;
            
            std::vector<OutputFormat> output_formats = {OutputFormat::CSV};
            std::vector<std::string> source_files;  // NEW: Paths to source files to save
        };

        explicit SimulationRun(const Config& config);
        ~SimulationRun() = default;

        // Directory access
        std::string get_run_directory() const { return m_run_directory; }
        std::string get_run_id() const { return m_run_id; }
        std::string get_outputs_directory(OutputFormat format) const;
        std::string get_visualizations_directory() const;
        std::string get_logs_directory() const;
        std::string get_analysis_directory() const;

        // File saving
        void save_initial_conditions(const std::vector<SPHParticle>& particles, 
                                     const UnitSystem& units);
        void save_config(const std::string& config_json);
        void save_metadata(const std::string& metadata_json);
        void save_source_files();  // NEW: Save source code files

        // Output writer creation
        std::unique_ptr<OutputWriter> create_writer(OutputFormat format, 
                                                    const UnitSystem& units);

        // Get config
        const Config& get_config() const { return m_config; }

    private:
        Config m_config;
        std::string m_run_id;
        std::string m_run_directory;
        std::chrono::system_clock::time_point m_start_time;

        std::string generate_run_id() const;
        void create_directory_structure();
        void create_latest_symlink();
        void create_directories(const std::string& path);
    };

    /**
     * @brief Generate metadata JSON for a simulation run
     */
    class MetadataGenerator
    {
    public:
        struct RunInfo
        {
            std::string run_id;
            std::string sample_name;
            std::string description;
            std::string created_at;
            std::string completed_at;
            double duration_seconds;
            std::string user;
            std::string hostname;
        };

        struct CodeVersion
        {
            std::string git_hash;
            std::string git_branch;
            bool git_dirty;
            std::string build_date;
            std::string compiler;
            std::string cmake_version;
        };

        struct SimulationParams
        {
            std::string sph_type;
            int dimension;
            int particle_count;
            real gamma;
            real cfl_sound;
            real end_time;
            real output_interval;
            int neighbor_number;
            bool use_balsara;
            bool use_time_dependent_av;
        };

        struct OutputInfo
        {
            std::vector<std::string> formats;
            int snapshot_count;
            std::vector<real> snapshot_times;
            double total_output_size_mb;
        };

        struct PerformanceInfo
        {
            int total_timesteps;
            double wall_time_seconds;
            double timesteps_per_second;
            double particles_per_second;
        };

        static std::string generate(
            const RunInfo& run_info,
            const CodeVersion& code_version,
            const SimulationParams& sim_params,
            const UnitSystem& units,
            const OutputInfo& output_info,
            const PerformanceInfo& perf_info
        );

    private:
        static std::string get_timestamp_iso8601();
        static std::string get_git_hash();
        static std::string get_git_branch();
        static bool is_git_dirty();
        static std::string get_username();
        static std::string get_hostname();
        static std::string get_compiler_version();
    };

} // namespace sph
