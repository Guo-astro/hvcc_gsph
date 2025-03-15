#pragma once

#include <fstream>
#include <memory>
#include <string>

#include "defines.hpp"
#include "unit_system.hpp" // Ensure UnitSystem is declared

namespace sph
{
    class SPHParticle;
    class Simulation;
    inline double safe_stod(const std::string &str, const std::string &field_name)
    {
        try
        {
            return std::stod(str);
        }
        catch (const std::invalid_argument &e)
        {
            WRITE_LOG << "Invalid argument for " << field_name << ": '" << str << "'";
            throw;
        }
        catch (const std::out_of_range &e)
        {
            WRITE_LOG << "Out of range for " << field_name << ": '" << str << "'";
            // throw;
        }
    }

    inline int safe_stoi(const std::string &str, const std::string &field_name)
    {
        try
        {
            return std::stoi(str);
        }
        catch (const std::invalid_argument &e)
        {
            WRITE_LOG << "Invalid argument for " << field_name << ": '" << str << "'";
            throw;
        }
        catch (const std::out_of_range &e)
        {
            WRITE_LOG << "Out of range for " << field_name << ": '" << str << "'";
            throw;
        }
    }
    class Output
    {
        int m_count;
        std::string m_dir;
        UnitSystem m_unit;
        std::ofstream m_out_energy;

        // Add the recenter flag as a member variable.
        bool m_recenterParticles;

    public:
        // Update the constructor to accept a recenter flag.
        Output(const std::string &dir, int count = 0, const UnitSystem &unit = UnitSystem(), bool recenterParticles = false);
        ~Output();
        void output_particle(std::shared_ptr<Simulation> sim);
        void output_energy(std::shared_ptr<Simulation> sim);
        void read_checkpoint(const std::string &file_name, std::shared_ptr<Simulation> sim);
        void recenterParticles(std::vector<SPHParticle> &particles);
    };
}
