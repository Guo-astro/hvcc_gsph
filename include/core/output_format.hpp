#pragma once

#include <string>
#include <memory>
#include <vector>
#include "utilities/defines.hpp"

namespace sph
{
    class SPHParticle;
    class Simulation;
    class UnitSystem;

    /**
     * @brief Output format types supported by the simulation
     */
    enum class OutputFormat
    {
        CSV,           // Plain text CSV (current format, human-readable)
        BINARY,        // Binary format (highly compressed, fast I/O)
        HDF5,          // HDF5 format (compressed, industry standard)
        NUMPY          // NumPy .npz format (Python-friendly, compressed)
    };

    /**
     * @brief Base class for output writers
     */
    class OutputWriter
    {
    protected:
        std::string m_directory;
        int m_snapshot_count;
        const UnitSystem& m_units;

    public:
        OutputWriter(const std::string& dir, int count, const UnitSystem& units)
            : m_directory(dir), m_snapshot_count(count), m_units(units) {}
        
        virtual ~OutputWriter() = default;
        
        /**
         * @brief Write particle snapshot to file
         * @param sim Simulation object containing particle data
         */
        virtual void write_snapshot(std::shared_ptr<Simulation> sim) = 0;
        
        /**
         * @brief Get file extension for this format
         */
        virtual std::string get_extension() const = 0;
        
        /**
         * @brief Get format name
         */
        virtual std::string get_format_name() const = 0;
        
        int get_snapshot_count() const { return m_snapshot_count; }
        void increment_count() { ++m_snapshot_count; }
    };

    /**
     * @brief CSV output writer (current format)
     */
    class CSVOutputWriter : public OutputWriter
    {
    public:
        using OutputWriter::OutputWriter;
        
        void write_snapshot(std::shared_ptr<Simulation> sim) override;
        std::string get_extension() const override { return ".csv"; }
        std::string get_format_name() const override { return "CSV"; }
    };

    /**
     * @brief Binary output writer (compact, fast)
     * Format: 
     * - Header: magic number, version, dimension, particle count, time, units
     * - Data: raw binary particle array
     */
    class BinaryOutputWriter : public OutputWriter
    {
    public:
        using OutputWriter::OutputWriter;
        
        void write_snapshot(std::shared_ptr<Simulation> sim) override;
        std::string get_extension() const override { return ".sph"; }
        std::string get_format_name() const override { return "Binary"; }
        
    private:
        static constexpr uint32_t MAGIC_NUMBER = 0x53504801; // "SPH\x01"
        static constexpr uint32_t VERSION = 1;
    };

    /**
     * @brief NumPy output writer (Python-friendly)
     * Writes compressed .npz file with named arrays
     */
    class NumpyOutputWriter : public OutputWriter
    {
    public:
        using OutputWriter::OutputWriter;
        
        void write_snapshot(std::shared_ptr<Simulation> sim) override;
        std::string get_extension() const override { return ".npz"; }
        std::string get_format_name() const override { return "NumPy"; }
    };

    /**
     * @brief Factory function to create appropriate output writer
     */
    std::unique_ptr<OutputWriter> create_output_writer(
        OutputFormat format,
        const std::string& directory,
        int count,
        const UnitSystem& units
    );

    /**
     * @brief Parse output format from string
     */
    OutputFormat parse_output_format(const std::string& format_str);

    /**
     * @brief Generate properly formatted output directory path
     * 
     * Creates path: {base_dir}/{sample_name}/{sph_type}/{dim}D/{format}/
     * Example: visualizations/shock_tube/DISPH/1D/csv/
     * 
     * @param base_dir Base directory (e.g., "visualizations")
     * @param sample_name Sample name (e.g., "shock_tube", "sedov_taylor")
     * @param sph_type SPH method (e.g., "DISPH", "GSPH")
     * @param dimension Dimension (1, 2, or 3)
     * @param format Output format
     * @param create_dir Whether to create the directory if it doesn't exist
     * @return Full path to output directory
     */
    std::string generate_output_path(
        const std::string& base_dir,
        const std::string& sample_name,
        const std::string& sph_type,
        int dimension,
        OutputFormat format,
        bool create_dir = true
    );

} // namespace sph
