#include "heating_cooling.hpp"
#include "parameters.hpp"
#include "simulation.hpp"
#include "particle.hpp"

namespace sph
{

    void HeatingCoolingModule::initialize(std::shared_ptr<SPHParameters> param)
    {
        m_is_valid = param->heating_cooling.is_valid;
        m_heating_rate = param->heating_cooling.heating_rate;
        m_cooling_rate = param->heating_cooling.cooling_rate;
    }

    void HeatingCoolingModule::calculation(std::shared_ptr<Simulation> sim)
    {
        if (!m_is_valid)
            return;

        auto &particles = sim->get_particles();
        const int num = sim->get_particle_num();
        const real dt = sim->get_dt();

#pragma omp parallel for
        for (int i = 0; i < num; ++i)
        {
            // Simple linear heating/cooling
            particles[i].ene += dt * (m_heating_rate - m_cooling_rate);
        }
    }

} // namespace sph
