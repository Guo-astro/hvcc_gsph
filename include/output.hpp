#pragma once

#include <fstream>
#include <memory>

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

    public:
        // Fix: Provide a default value for the 'unit' parameter.
        Output(const std::string &dir, int count = 0, const UnitSystem &unit = UnitSystem());
        ~Output();
        void output_particle(std::shared_ptr<Simulation> sim);
        void output_energy(std::shared_ptr<Simulation> sim);
    };

}
