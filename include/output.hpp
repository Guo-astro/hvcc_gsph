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
