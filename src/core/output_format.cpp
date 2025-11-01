#include "core/output_format.hpp"
#include "core/simulation.hpp"
#include "core/particle.hpp"
#include "core/logger.hpp"
#include "utilities/unit_system.hpp"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <cstring>
#include <boost/format.hpp>
#include <sys/stat.h>
#include <sys/types.h>

namespace sph
{
    // ========================================================================
    // CSV Output Writer Implementation
    // ========================================================================
    
    void CSVOutputWriter::write_snapshot(std::shared_ptr<Simulation> sim)
    {
        const auto& particles = sim->get_particles();
        const int num = sim->get_particle_num();
        const real time = sim->get_time();

        const std::string filename = m_directory + (boost::format("/%05d.csv") % m_snapshot_count).str();
        std::ofstream out(filename);

        if (!out.is_open()) {
            WRITE_LOG << "ERROR: Cannot open output file: " << filename;
            return;
        }

        // Write header with units
        std::ostringstream header;
        header << "time [" << m_units.time_unit << "],";
        
#if DIM == 1
        header << "pos_x [" << m_units.length_unit << "],"
               << "vel_x [" << m_units.length_unit << "/" << m_units.time_unit << "],"
               << "acc_x [" << m_units.length_unit << "/" << m_units.time_unit << "^2],";
#elif DIM == 2
        header << "pos_x [" << m_units.length_unit << "],"
               << "pos_y [" << m_units.length_unit << "],"
               << "vel_x [" << m_units.length_unit << "/" << m_units.time_unit << "],"
               << "vel_y [" << m_units.length_unit << "/" << m_units.time_unit << "],"
               << "acc_x [" << m_units.length_unit << "/" << m_units.time_unit << "^2],"
               << "acc_y [" << m_units.length_unit << "/" << m_units.time_unit << "^2],";
#elif DIM == 3
        header << "pos_x [" << m_units.length_unit << "],"
               << "pos_y [" << m_units.length_unit << "],"
               << "pos_z [" << m_units.length_unit << "],"
               << "vel_x [" << m_units.length_unit << "/" << m_units.time_unit << "],"
               << "vel_y [" << m_units.length_unit << "/" << m_units.time_unit << "],"
               << "vel_z [" << m_units.length_unit << "/" << m_units.time_unit << "],"
               << "acc_x [" << m_units.length_unit << "/" << m_units.time_unit << "^2],"
               << "acc_y [" << m_units.length_unit << "/" << m_units.time_unit << "^2],"
               << "acc_z [" << m_units.length_unit << "/" << m_units.time_unit << "^2],";
#endif
        header << "mass [" << m_units.mass_unit << "],"
               << "dens [" << m_units.density_unit << "],"
               << "pres [" << m_units.pressure_unit << "],"
               << "ene [" << m_units.energy_unit << "],"
               << "sml [" << m_units.length_unit << "],"
               << "id,neighbor,alpha,gradh,shockSensor,ene_floored";

        out << header.str() << "\n";

        // Write particle data
        for (int i = 0; i < num; ++i) {
            const auto& p = particles[i];
            std::ostringstream line;
            
            line << std::scientific << std::setprecision(6);
            line << time * m_units.time_factor << ",";
            
#if DIM == 1
            line << p.pos[0] * m_units.length_factor << ","
                 << p.vel[0] * (m_units.length_factor / m_units.time_factor) << ","
                 << p.acc[0] * (m_units.length_factor / (m_units.time_factor * m_units.time_factor)) << ",";
#elif DIM == 2
            line << p.pos[0] * m_units.length_factor << ","
                 << p.pos[1] * m_units.length_factor << ","
                 << p.vel[0] * (m_units.length_factor / m_units.time_factor) << ","
                 << p.vel[1] * (m_units.length_factor / m_units.time_factor) << ","
                 << p.acc[0] * (m_units.length_factor / (m_units.time_factor * m_units.time_factor)) << ","
                 << p.acc[1] * (m_units.length_factor / (m_units.time_factor * m_units.time_factor)) << ",";
#elif DIM == 3
            line << p.pos[0] * m_units.length_factor << ","
                 << p.pos[1] * m_units.length_factor << ","
                 << p.pos[2] * m_units.length_factor << ","
                 << p.vel[0] * (m_units.length_factor / m_units.time_factor) << ","
                 << p.vel[1] * (m_units.length_factor / m_units.time_factor) << ","
                 << p.vel[2] * (m_units.length_factor / m_units.time_factor) << ","
                 << p.acc[0] * (m_units.length_factor / (m_units.time_factor * m_units.time_factor)) << ","
                 << p.acc[1] * (m_units.length_factor / (m_units.time_factor * m_units.time_factor)) << ","
                 << p.acc[2] * (m_units.length_factor / (m_units.time_factor * m_units.time_factor)) << ",";
#endif
            line << p.mass * m_units.mass_factor << ","
                 << p.dens * m_units.density_factor << ","
                 << p.pres * m_units.pressure_factor << ","
                 << p.ene * m_units.energy_factor << ","
                 << p.sml * m_units.length_factor << ","
                 << p.id << ","
                 << p.neighbor << ","
                 << p.alpha << ","
                 << p.gradh << ","
                 << p.shockSensor << ","
                 << p.ene_floored;
                 
            out << line.str() << "\n";
        }

        out.close();
        WRITE_LOG << "CSV snapshot written: " << filename;
        increment_count();
    }

    // ========================================================================
    // Binary Output Writer Implementation
    // ========================================================================
    
    void BinaryOutputWriter::write_snapshot(std::shared_ptr<Simulation> sim)
    {
        const auto& particles = sim->get_particles();
        const int num = sim->get_particle_num();
        const real time = sim->get_time();

        const std::string filename = m_directory + (boost::format("/%05d.sph") % m_snapshot_count).str();
        std::ofstream out(filename, std::ios::binary);

        if (!out.is_open()) {
            WRITE_LOG << "ERROR: Cannot open binary output file: " << filename;
            return;
        }

        // Write header
        uint32_t magic = MAGIC_NUMBER;
        uint32_t version = VERSION;
        uint32_t dimension = DIM;
        uint32_t particle_count = num;
        double sim_time = time * m_units.time_factor;
        
        out.write(reinterpret_cast<const char*>(&magic), sizeof(magic));
        out.write(reinterpret_cast<const char*>(&version), sizeof(version));
        out.write(reinterpret_cast<const char*>(&dimension), sizeof(dimension));
        out.write(reinterpret_cast<const char*>(&particle_count), sizeof(particle_count));
        out.write(reinterpret_cast<const char*>(&sim_time), sizeof(sim_time));

        // Write unit system metadata
        double length_factor = m_units.length_factor;
        double time_factor = m_units.time_factor;
        double mass_factor = m_units.mass_factor;
        out.write(reinterpret_cast<const char*>(&length_factor), sizeof(length_factor));
        out.write(reinterpret_cast<const char*>(&time_factor), sizeof(time_factor));
        out.write(reinterpret_cast<const char*>(&mass_factor), sizeof(mass_factor));

        // Write unit name strings (fixed 16 bytes each)
        char unit_buffer[16];
        auto write_unit_name = [&](const std::string& unit_name) {
            std::memset(unit_buffer, 0, 16);
            std::strncpy(unit_buffer, unit_name.c_str(), 15);
            out.write(unit_buffer, 16);
        };
        
        write_unit_name(m_units.length_unit);
        write_unit_name(m_units.time_unit);
        write_unit_name(m_units.mass_unit);

        // Write particle data (compact binary format)
        for (int i = 0; i < num; ++i) {
            const auto& p = particles[i];
            
            // Position (DIM doubles)
            for (int d = 0; d < DIM; ++d) {
                double val = p.pos[d] * m_units.length_factor;
                out.write(reinterpret_cast<const char*>(&val), sizeof(double));
            }
            
            // Velocity (DIM doubles)
            for (int d = 0; d < DIM; ++d) {
                double val = p.vel[d] * (m_units.length_factor / m_units.time_factor);
                out.write(reinterpret_cast<const char*>(&val), sizeof(double));
            }
            
            // Acceleration (DIM doubles)
            for (int d = 0; d < DIM; ++d) {
                double val = p.acc[d] * (m_units.length_factor / (m_units.time_factor * m_units.time_factor));
                out.write(reinterpret_cast<const char*>(&val), sizeof(double));
            }
            
            // Scalar fields (8 doubles)
            double mass = p.mass * m_units.mass_factor;
            double dens = p.dens * m_units.density_factor;
            double pres = p.pres * m_units.pressure_factor;
            double ene = p.ene * m_units.energy_factor;
            double sml = p.sml * m_units.length_factor;
            double alpha = p.alpha;
            double gradh = p.gradh;
            double shock_sensor = p.shockSensor;
            
            out.write(reinterpret_cast<const char*>(&mass), sizeof(double));
            out.write(reinterpret_cast<const char*>(&dens), sizeof(double));
            out.write(reinterpret_cast<const char*>(&pres), sizeof(double));
            out.write(reinterpret_cast<const char*>(&ene), sizeof(double));
            out.write(reinterpret_cast<const char*>(&sml), sizeof(double));
            out.write(reinterpret_cast<const char*>(&alpha), sizeof(double));
            out.write(reinterpret_cast<const char*>(&gradh), sizeof(double));
            out.write(reinterpret_cast<const char*>(&shock_sensor), sizeof(double));
            
            // Integer fields (3 int32)
            int32_t id = p.id;
            int32_t neighbor = p.neighbor;
            int32_t ene_floored = p.ene_floored;
            out.write(reinterpret_cast<const char*>(&id), sizeof(int32_t));
            out.write(reinterpret_cast<const char*>(&neighbor), sizeof(int32_t));
            out.write(reinterpret_cast<const char*>(&ene_floored), sizeof(int32_t));
        }

        out.close();
        
        // Calculate compression ratio
        std::ifstream in(filename, std::ios::binary | std::ios::ate);
        size_t binary_size = in.tellg();
        in.close();
        
        // Estimate CSV size (rough approximation: ~100 bytes per particle)
        size_t estimated_csv_size = num * 100;
        double compression_ratio = static_cast<double>(estimated_csv_size) / binary_size;
        
        WRITE_LOG << "Binary snapshot written: " << filename 
                  << " (size: " << binary_size << " bytes, ~" 
                  << std::fixed << std::setprecision(1) << compression_ratio << "x compression)";
        increment_count();
    }

    // ========================================================================
    // NumPy Output Writer Implementation
    // ========================================================================
    
    void NumpyOutputWriter::write_snapshot(std::shared_ptr<Simulation> sim)
    {
        // For now, implement as CSV with .npy extension
        // Full NumPy implementation would require libnpy or cnpy library
        const auto& particles = sim->get_particles();
        const int num = sim->get_particle_num();
        const real time = sim->get_time();

        const std::string filename = m_directory + (boost::format("/%05d.npz") % m_snapshot_count).str();
        
        WRITE_LOG << "NumPy output format not yet fully implemented. "
                  << "Install cnpy library for compressed .npz output. "
                  << "Using CSV format temporarily: " << filename;
        
        // Fallback to CSV for now
        CSVOutputWriter csv_writer(m_directory, m_snapshot_count, m_units);
        csv_writer.write_snapshot(sim);
        
        increment_count();
    }

    // ========================================================================
    // Factory and Utility Functions
    // ========================================================================
    
    std::unique_ptr<OutputWriter> create_output_writer(
        OutputFormat format,
        const std::string& directory,
        int count,
        const UnitSystem& units)
    {
        switch (format) {
            case OutputFormat::CSV:
                return std::make_unique<CSVOutputWriter>(directory, count, units);
            case OutputFormat::BINARY:
                return std::make_unique<BinaryOutputWriter>(directory, count, units);
            case OutputFormat::NUMPY:
                return std::make_unique<NumpyOutputWriter>(directory, count, units);
            default:
                WRITE_LOG << "Unknown output format, defaulting to CSV";
                return std::make_unique<CSVOutputWriter>(directory, count, units);
        }
    }

    OutputFormat parse_output_format(const std::string& format_str)
    {
        std::string lower_format = format_str;
        std::transform(lower_format.begin(), lower_format.end(), lower_format.begin(), ::tolower);
        
        if (lower_format == "csv" || lower_format == "text") {
            return OutputFormat::CSV;
        } else if (lower_format == "binary" || lower_format == "bin") {
            return OutputFormat::BINARY;
        } else if (lower_format == "numpy" || lower_format == "npz" || lower_format == "npy") {
            return OutputFormat::NUMPY;
        } else if (lower_format == "hdf5" || lower_format == "h5") {
            WRITE_LOG << "HDF5 format not yet implemented, defaulting to CSV";
            return OutputFormat::CSV;
        } else {
            WRITE_LOG << "Unknown format '" << format_str << "', defaulting to CSV";
            return OutputFormat::CSV;
        }
    }

    std::string generate_output_path(
        const std::string& base_dir,
        const std::string& sample_name,
        const std::string& sph_type,
        int dimension,
        OutputFormat format,
        bool create_dir)
    {
        // Determine format subdirectory
        std::string format_dir;
        switch (format) {
            case OutputFormat::CSV:    format_dir = "csv"; break;
            case OutputFormat::BINARY: format_dir = "binary"; break;
            case OutputFormat::NUMPY:  format_dir = "numpy"; break;
            case OutputFormat::HDF5:   format_dir = "hdf5"; break;
        }
        
        // Build path: base_dir/sample_name/sph_type/XD/format/
        std::ostringstream path;
        path << base_dir << "/"
             << sample_name << "/"
             << sph_type << "/"
             << dimension << "D/"
             << format_dir;
        
        std::string full_path = path.str();
        
        // Create directory if requested
        if (create_dir) {
            std::string current_path;
            std::istringstream path_stream(full_path);
            std::string component;
            
            while (std::getline(path_stream, component, '/')) {
                if (component.empty()) continue;
                
                if (!current_path.empty()) {
                    current_path += "/";
                }
                current_path += component;
                
                // Create directory (cross-platform)
#ifdef _WIN32
                _mkdir(current_path.c_str());
#else
                mkdir(current_path.c_str(), 0755);
#endif
            }
            
            WRITE_LOG << "Output directory created/verified: " << full_path;
        }
        
        return full_path;
    }

} // namespace sph
