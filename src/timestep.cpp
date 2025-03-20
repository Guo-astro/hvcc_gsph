#include "parameters.hpp"
#include "timestep.hpp"
#include "particle.hpp"
#include "simulation.hpp"
#include "openmp.hpp"
#include "exception.hpp" // Assuming WRITE_LOG is defined here

#include <algorithm> // For std::min
#include <limits>    // For std::numeric_limits
#include <string>    // For std::string

namespace sph
{
    void TimeStep::initialize(std::shared_ptr<SPHParameters> param)
    {
        c_sound = param->cfl.sound;
        c_force = param->cfl.force;
        c_ene = param->cfl.ene;
    }

    void TimeStep::calculation(std::shared_ptr<Simulation> sim)
    {
        auto &particles = sim->get_particles();
        const int num = sim->get_particle_num();

        // Variables to find the minimum timesteps across particles
        omp_real dt_min_force(std::numeric_limits<real>::max());
        omp_real dt_min_ene(std::numeric_limits<real>::max());

        // Parallel loop to compute force and energy timesteps
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
            if (dene_abs > 0.0 && ene_abs > 1e-10)
            {
                const real dt_ene_i = c_ene * ene_abs / dene_abs;
                if (dt_ene_i < dt_min_ene.get())
                {
                    dt_min_ene.get() = dt_ene_i;
                }
            }
        }

        // Compute the sound timestep
        const real dt_sound_i = c_sound * sim->get_h_per_v_sig();
        const real dt_force = dt_min_force.min();
        const real dt_ene = dt_min_ene.min();

        // Set the global timestep as the smallest of dt_sound_i and dt_force
        real dt_global = std::min({dt_sound_i, dt_force});

        // Apply the condition: if dt_global < 1e-3, multiply by 5
        const real dt_threshold = 1e-5;
        if (dt_global < dt_threshold)
        {
            real dt_original = dt_global; // Store original for logging
            dt_global *= 5;               // Multiply by 5
            WRITE_LOG << "Warning: dt_global adjusted from " << dt_original
                      << " to " << dt_global << " (multiplied by 5) at t = " << sim->get_time();
        }

        // Set the adjusted timestep in the simulation
        sim->set_dt(dt_global);

        // Determine which criterion is limiting
        std::string limiting_criterion;
        if (dt_global == dt_sound_i)
        {
            limiting_criterion = "sound";
        }
        else if (dt_global == dt_force)
        {
            limiting_criterion = "force";
        }

        // Log the timestep values and the limiting criterion
        WRITE_LOG << "Timestep criteria at t = " << sim->get_time() << ": "
                  << "dt_sound = " << dt_sound_i << ", "
                  << "dt_force = " << dt_force << ", "
                  << "dt_ene = " << dt_ene << ". "
                  << "Limiting criterion(ene is only for reference): " << limiting_criterion;
    }
}