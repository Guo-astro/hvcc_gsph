#include <cmath>
#include <omp.h>
#include "core/simulation.hpp"
#include "core/parameters.hpp"
#include "core/particle.hpp"
#include "utilities/defines.hpp"

// This file provides alternative integration routines for a simulation
// in which fluid particles are constrained to remain in the x-y plane
// (i.e. z=0 and vz=0). Gravity is computed in full 3D.

namespace sph
{

    /// In-plane predict step: update positions and half-step velocities/energies,
    /// then enforce that z and vz are clamped to 0 for non-wall particles.
    void inplane_predict(std::shared_ptr<Simulation> sim)
    {
        auto &particles = sim->get_particles();
        const int num = sim->get_particle_num();
        const real dt = sim->get_dt();

        // Get gamma from parameters. Here we assume the same gamma applies.
        // (In a more modular design, you might pass gamma as a parameter.)
        // For demonstration, we assume gamma is stored in sim->get_particles()[0].ene etc.
        // Alternatively, you can pass gamma from your parameter structure.
        const real gamma = 5.0 / 3.0;
        const real c_sound_factor = gamma * (gamma - 1.0);

#pragma omp parallel for
        for (int i = 0; i < num; ++i)
        {
            // Skip wall particles (they are immovable)
            if (particles[i].is_wall)
                continue;

            // Standard half-step prediction:
            particles[i].vel_p = particles[i].vel + particles[i].acc * (0.5 * dt);
            particles[i].ene_p = particles[i].ene + particles[i].dene * (0.5 * dt);

            // Update positions and velocities:
            particles[i].pos += particles[i].vel_p * dt;
            particles[i].vel += particles[i].acc * dt;
            particles[i].ene += particles[i].dene * dt;
            particles[i].sound = std::sqrt(c_sound_factor * particles[i].ene);

            // --- Enforce in-plane constraint ---
            particles[i].pos[2] = 0.0;
            particles[i].vel[2] = 0.0;
        }
    }

    /// In-plane correct step: complete the integration and again enforce the plane.
    void inplane_correct(std::shared_ptr<Simulation> sim)
    {
        auto &particles = sim->get_particles();
        const int num = sim->get_particle_num();
        const real dt = sim->get_dt();
        const real gamma = 5.0 / 3.0;
        const real c_sound_factor = gamma * (gamma - 1.0);

#pragma omp parallel for
        for (int i = 0; i < num; ++i)
        {
            // Skip wall particles
            if (particles[i].is_wall)
                continue;

            // Correction step:
            particles[i].vel = particles[i].vel_p + particles[i].acc * (0.5 * dt);
            particles[i].ene = particles[i].ene_p + particles[i].dene * (0.5 * dt);
            particles[i].sound = std::sqrt(c_sound_factor * particles[i].ene);

            // Enforce in-plane motion:
            particles[i].pos[2] = 0.0;
            particles[i].vel[2] = 0.0;
        }
    }

} // namespace sph