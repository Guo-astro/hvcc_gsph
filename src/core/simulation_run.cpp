#include "core/simulation_run.hpp"
#include "core/simulation.hpp"
#include "core/parameters.hpp"
#include "core/particle.hpp"
#include "core/logger.hpp"
#include "utilities/unit_system.hpp"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <sys/stat.h>
#include <unistd.h>
#include <pwd.h>

#ifdef __APPLE__
#include <sys/sysctl.h>
#endif

namespace sph
{
    // ========================================================================
    // SimulationRun Implementation
    // ========================================================================

    SimulationRun::SimulationRun(const Config& config)
        : m_config(config)
        , m_start_time(std::chrono::system_clock::now())
    {
        // Generate run ID
        if (config.auto_run_id) {
            m_run_id = generate_run_id();
        } else {
            m_run_id = config.custom_run_id.empty() ? "custom_run" : config.custom_run_id;
        }

        // Build run directory path
        std::ostringstream path;
        path << config.base_dir << "/" << config.sample_name << "/" << m_run_id;
        m_run_directory = path.str();

        // Create directory structure
        create_directory_structure();

        // Create latest symlink
        if (config.create_latest_symlink) {
            create_latest_symlink();
        }

        WRITE_LOG << "Simulation run directory: " << m_run_directory;
    }

    std::string SimulationRun::generate_run_id() const
    {
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        auto tm = *std::localtime(&time_t);

        std::ostringstream oss;
        oss << "run_"
            << std::put_time(&tm, "%Y-%m-%d_%H%M%S")
            << "_" << m_config.sph_type;
        
        // Add dimension suffix
        oss << "_" << m_config.dimension << "d";

        return oss.str();
    }

    void SimulationRun::create_directories(const std::string& path)
    {
        std::string current_path;
        std::istringstream path_stream(path);
        std::string component;

        while (std::getline(path_stream, component, '/')) {
            if (component.empty()) continue;

            if (!current_path.empty()) {
                current_path += "/";
            }
            current_path += component;

#ifdef _WIN32
            _mkdir(current_path.c_str());
#else
            mkdir(current_path.c_str(), 0755);
#endif
        }
    }

    void SimulationRun::create_directory_structure()
    {
        // Create main run directory
        create_directories(m_run_directory);

        // Create output format directories
        for (auto format : m_config.output_formats) {
            create_directories(get_outputs_directory(format));
        }

        // Create other directories
        create_directories(get_visualizations_directory());
        create_directories(get_logs_directory());
        create_directories(get_analysis_directory());
        
        // Create source code directory
        if (m_config.save_source_code) {
            create_directories(m_run_directory + "/source");
        }
    }

    void SimulationRun::create_latest_symlink()
    {
#ifndef _WIN32
        std::string sample_dir = m_config.base_dir + "/" + m_config.sample_name;
        std::string latest_link = sample_dir + "/latest";

        // Remove old symlink if exists
        unlink(latest_link.c_str());

        // Create new symlink (relative path)
        if (symlink(m_run_id.c_str(), latest_link.c_str()) == 0) {
            WRITE_LOG << "Created latest symlink: " << latest_link << " -> " << m_run_id;
        }
#endif
    }

    std::string SimulationRun::get_outputs_directory(OutputFormat format) const
    {
        std::string format_str;
        switch (format) {
            case OutputFormat::CSV:    format_str = "csv"; break;
            case OutputFormat::BINARY: format_str = "binary"; break;
            case OutputFormat::NUMPY:  format_str = "numpy"; break;
            case OutputFormat::HDF5:   format_str = "hdf5"; break;
        }
        return m_run_directory + "/outputs/" + format_str;
    }

    std::string SimulationRun::get_visualizations_directory() const
    {
        return m_run_directory + "/visualizations";
    }

    std::string SimulationRun::get_logs_directory() const
    {
        return m_run_directory + "/logs";
    }

    std::string SimulationRun::get_analysis_directory() const
    {
        return m_run_directory + "/analysis";
    }

    void SimulationRun::save_initial_conditions(
        const std::vector<SPHParticle>& particles,
        const UnitSystem& units)
    {
        std::string filename = m_run_directory + "/initial_conditions.csv";
        std::ofstream out(filename);

        if (!out.is_open()) {
            WRITE_LOG << "ERROR: Cannot save initial conditions to " << filename;
            return;
        }

        // Write header
        out << "time [" << units.time_unit << "],";
#if DIM == 1
        out << "pos_x [" << units.length_unit << "],";
#elif DIM == 2
        out << "pos_x [" << units.length_unit << "],"
            << "pos_y [" << units.length_unit << "],";
#elif DIM == 3
        out << "pos_x [" << units.length_unit << "],"
            << "pos_y [" << units.length_unit << "],"
            << "pos_z [" << units.length_unit << "],";
#endif
        out << "mass [" << units.mass_unit << "],"
            << "dens [" << units.density_unit << "],"
            << "pres [" << units.pressure_unit << "],"
            << "ene [" << units.energy_unit << "]\n";

        // Write particle data
        for (const auto& p : particles) {
            out << "0.0,";
#if DIM >= 1
            out << p.pos[0] * units.length_factor << ",";
#endif
#if DIM >= 2
            out << p.pos[1] * units.length_factor << ",";
#endif
#if DIM == 3
            out << p.pos[2] * units.length_factor << ",";
#endif
            out << p.mass * units.mass_factor << ","
                << p.dens * units.density_factor << ","
                << p.pres * units.pressure_factor << ","
                << p.ene * units.energy_factor << "\n";
        }

        out.close();
        WRITE_LOG << "Initial conditions saved: " << filename;
    }

    void SimulationRun::save_config(const std::string& config_json)
    {
        std::string filename = m_run_directory + "/config.json";
        std::ofstream out(filename);

        if (!out.is_open()) {
            WRITE_LOG << "ERROR: Cannot save config to " << filename;
            return;
        }

        out << config_json;
        out.close();
        WRITE_LOG << "Config saved: " << filename;
    }

    void SimulationRun::save_metadata(const std::string& metadata_json)
    {
        std::string filename = m_run_directory + "/metadata.json";
        std::ofstream out(filename);

        if (!out.is_open()) {
            WRITE_LOG << "ERROR: Cannot save metadata to " << filename;
            return;
        }

        out << metadata_json;
        out.close();
        WRITE_LOG << "Metadata saved: " << filename;
    }

    void SimulationRun::save_source_files()
    {
        if (!m_config.save_source_code) {
            return;
        }

        std::string source_dir = m_run_directory + "/source";
        
        // If specific source files are registered, save those
        if (!m_config.source_files.empty()) {
            for (const auto& source_path : m_config.source_files) {
                std::ifstream src(source_path, std::ios::binary);
                if (!src.is_open()) {
                    WRITE_LOG << "WARNING: Cannot open source file: " << source_path;
                    continue;
                }

                // Extract filename from path
                size_t last_slash = source_path.find_last_of("/\\");
                std::string filename = (last_slash != std::string::npos) 
                    ? source_path.substr(last_slash + 1) 
                    : source_path;

                std::string dest_path = source_dir + "/" + filename;
                std::ofstream dest(dest_path, std::ios::binary);
                
                if (!dest.is_open()) {
                    WRITE_LOG << "WARNING: Cannot write source file: " << dest_path;
                    continue;
                }

                // Copy file
                dest << src.rdbuf();
                src.close();
                dest.close();
                
                WRITE_LOG << "Saved source file: " << filename;
            }
        } 
        // Otherwise, copy the entire src/samples directory tree
        else {
            // Try to find src/samples directory relative to executable
            // The executable is typically in build/, so src/samples is ../src/samples
            std::string samples_dir = "../src/samples";
            
            // Use system command to copy entire directory tree
            std::string copy_cmd = "cp -r " + samples_dir + " " + source_dir + "/samples 2>/dev/null";
            int result = system(copy_cmd.c_str());
            
            if (result == 0) {
                WRITE_LOG << "Saved entire samples source tree to: " << source_dir << "/samples";
            } else {
                WRITE_LOG << "WARNING: Could not copy samples directory from: " << samples_dir;
            }
        }
    }

    std::unique_ptr<OutputWriter> SimulationRun::create_writer(
        OutputFormat format,
        const UnitSystem& units)
    {
        std::string output_dir = get_outputs_directory(format);
        return create_output_writer(format, output_dir, 0, units);
    }

    // ========================================================================
    // MetadataGenerator Implementation
    // ========================================================================

    std::string MetadataGenerator::get_timestamp_iso8601()
    {
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        auto tm = *std::gmtime(&time_t);

        std::ostringstream oss;
        oss << std::put_time(&tm, "%Y-%m-%dT%H:%M:%SZ");
        return oss.str();
    }

    std::string MetadataGenerator::get_git_hash()
    {
#ifdef GIT_HASH
        return GIT_HASH;
#else
        // Try to get git hash at runtime
        FILE* pipe = popen("git rev-parse --short HEAD 2>/dev/null", "r");
        if (pipe) {
            char buffer[128];
            std::string result;
            if (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
                result = buffer;
                // Remove newline
                if (!result.empty() && result.back() == '\n') {
                    result.pop_back();
                }
            }
            pclose(pipe);
            return result.empty() ? "unknown" : result;
        }
        return "unknown";
#endif
    }

    std::string MetadataGenerator::get_git_branch()
    {
        FILE* pipe = popen("git rev-parse --abbrev-ref HEAD 2>/dev/null", "r");
        if (pipe) {
            char buffer[128];
            std::string result;
            if (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
                result = buffer;
                if (!result.empty() && result.back() == '\n') {
                    result.pop_back();
                }
            }
            pclose(pipe);
            return result.empty() ? "unknown" : result;
        }
        return "unknown";
    }

    bool MetadataGenerator::is_git_dirty()
    {
        FILE* pipe = popen("git diff-index --quiet HEAD -- 2>/dev/null", "r");
        if (pipe) {
            int status = pclose(pipe);
            return status != 0;
        }
        return false;
    }

    std::string MetadataGenerator::get_username()
    {
#ifndef _WIN32
        struct passwd* pw = getpwuid(getuid());
        if (pw) {
            return pw->pw_name;
        }
#endif
        return "unknown";
    }

    std::string MetadataGenerator::get_hostname()
    {
        char hostname[256];
        if (gethostname(hostname, sizeof(hostname)) == 0) {
            return hostname;
        }
        return "unknown";
    }

    std::string MetadataGenerator::get_compiler_version()
    {
#if defined(__clang__)
        return "Clang " + std::to_string(__clang_major__) + "." + 
               std::to_string(__clang_minor__) + "." + 
               std::to_string(__clang_patchlevel__);
#elif defined(__GNUC__)
        return "GCC " + std::to_string(__GNUC__) + "." + 
               std::to_string(__GNUC_MINOR__) + "." + 
               std::to_string(__GNUC_PATCHLEVEL__);
#elif defined(_MSC_VER)
        return "MSVC " + std::to_string(_MSC_VER);
#else
        return "Unknown";
#endif
    }

    std::string MetadataGenerator::generate(
        const RunInfo& run_info,
        const CodeVersion& code_version,
        const SimulationParams& sim_params,
        const UnitSystem& units,
        const OutputInfo& output_info,
        const PerformanceInfo& perf_info)
    {
        std::ostringstream json;
        json << std::fixed << std::setprecision(6);

        json << "{\n";
        
        // Run info
        json << "  \"run_info\": {\n";
        json << "    \"run_id\": \"" << run_info.run_id << "\",\n";
        json << "    \"sample_name\": \"" << run_info.sample_name << "\",\n";
        json << "    \"description\": \"" << run_info.description << "\",\n";
        json << "    \"created_at\": \"" << run_info.created_at << "\",\n";
        json << "    \"completed_at\": \"" << run_info.completed_at << "\",\n";
        json << "    \"duration_seconds\": " << run_info.duration_seconds << ",\n";
        json << "    \"user\": \"" << run_info.user << "\",\n";
        json << "    \"hostname\": \"" << run_info.hostname << "\"\n";
        json << "  },\n";

        // Code version
        json << "  \"code_version\": {\n";
        json << "    \"git_hash\": \"" << code_version.git_hash << "\",\n";
        json << "    \"git_branch\": \"" << code_version.git_branch << "\",\n";
        json << "    \"git_dirty\": " << (code_version.git_dirty ? "true" : "false") << ",\n";
        json << "    \"compiler\": \"" << code_version.compiler << "\"\n";
        json << "  },\n";

        // Simulation parameters
        json << "  \"simulation_params\": {\n";
        json << "    \"sph_type\": \"" << sim_params.sph_type << "\",\n";
        json << "    \"dimension\": " << sim_params.dimension << ",\n";
        json << "    \"particle_count\": " << sim_params.particle_count << ",\n";
        json << "    \"gamma\": " << sim_params.gamma << ",\n";
        json << "    \"cfl_sound\": " << sim_params.cfl_sound << ",\n";
        json << "    \"end_time\": " << sim_params.end_time << ",\n";
        json << "    \"output_interval\": " << sim_params.output_interval << ",\n";
        json << "    \"neighbor_number\": " << sim_params.neighbor_number << ",\n";
        json << "    \"use_balsara\": " << (sim_params.use_balsara ? "true" : "false") << ",\n";
        json << "    \"use_time_dependent_av\": " << (sim_params.use_time_dependent_av ? "true" : "false") << "\n";
        json << "  },\n";

        // Unit system
        json << "  \"unit_system\": {\n";
        json << "    \"length_unit\": \"" << units.length_unit << "\",\n";
        json << "    \"time_unit\": \"" << units.time_unit << "\",\n";
        json << "    \"mass_unit\": \"" << units.mass_unit << "\",\n";
        json << "    \"length_factor\": " << units.length_factor << ",\n";
        json << "    \"time_factor\": " << units.time_factor << ",\n";
        json << "    \"mass_factor\": " << units.mass_factor << "\n";
        json << "  },\n";

        // Output info
        json << "  \"output\": {\n";
        json << "    \"formats\": [";
        for (size_t i = 0; i < output_info.formats.size(); ++i) {
            json << "\"" << output_info.formats[i] << "\"";
            if (i < output_info.formats.size() - 1) json << ", ";
        }
        json << "],\n";
        json << "    \"snapshot_count\": " << output_info.snapshot_count << ",\n";
        json << "    \"snapshot_times\": [";
        for (size_t i = 0; i < output_info.snapshot_times.size(); ++i) {
            json << output_info.snapshot_times[i];
            if (i < output_info.snapshot_times.size() - 1) json << ", ";
        }
        json << "],\n";
        json << "    \"total_output_size_mb\": " << output_info.total_output_size_mb << "\n";
        json << "  },\n";

        // Performance
        json << "  \"performance\": {\n";
        json << "    \"total_timesteps\": " << perf_info.total_timesteps << ",\n";
        json << "    \"wall_time_seconds\": " << perf_info.wall_time_seconds << ",\n";
        json << "    \"timesteps_per_second\": " << perf_info.timesteps_per_second << ",\n";
        json << "    \"particles_per_second\": " << perf_info.particles_per_second << "\n";
        json << "  }\n";

        json << "}\n";

        return json.str();
    }

} // namespace sph
