#include "parameters.hpp"
#include "timestep.hpp"
#include "particle.hpp"
#include "simulation.hpp"
#include "openmp.hpp"

#include <algorithm>
#include <limits> // For std::numeric_limits

namespace sph
{

    void TimeStep::initialize(std::shared_ptr<SPHParameters> param)
    {
        c_sound = param->cfl.sound;
        c_force = param->cfl.force;
        c_ene = param->cfl.ene; // New safety factor for energy criterion
    }

    void TimeStep::calculation(std::shared_ptr<Simulation> sim)
    {
        auto &particles = sim->get_particles();
        const int num = sim->get_particle_num();

        omp_real dt_min_force(std::numeric_limits<real>::max()); // Minimum timestep for force criterion
        omp_real dt_min_ene(std::numeric_limits<real>::max());   // Minimum timestep for energy criterion

#pragma omp parallel for
        for (int i = 0; i < num; ++i)
        {
            // Force criterion
            const real acc_abs = std::abs(particles[i].acc);
            if (acc_abs > 0.0)
            {
                const real dt_force_i = c_force * std::sqrt(particles[i].sml / acc_abs);
                if (dt_force_i < dt_min_force.get())
                {
                    dt_min_force.get() = dt_force_i;
                }
            }

            // Energy criterion
            const real ene_abs = std::abs(particles[i].ene);
            const real dene_abs = std::abs(particles[i].dene);
            if (dene_abs > 0.0 && ene_abs > 1e-10) // Prevent division by zero and tiny ene
            {
                const real dt_ene_i = c_ene * ene_abs / dene_abs;
                if (dt_ene_i < dt_min_ene.get())
                {
                    dt_min_ene.get() = dt_ene_i;
                }
            }
        }

        const real dt_sound_i = c_sound * sim->get_h_per_v_sig(); // Sound speed criterion
        const real dt_force = dt_min_force.min();                 // Global min for force
        const real dt_ene = dt_min_ene.min();                     // Global min for energy

        // Set the global timestep as the minimum of all criteria
        sim->set_dt(std::min({dt_sound_i, dt_force, dt_ene}));
    }

}