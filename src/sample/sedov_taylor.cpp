#include <cmath>
#include <vector>
#include "sample_registry.hpp"
#include "simulation.hpp"
#include "parameters.hpp"
#include "particle.hpp"
#include "exception.hpp"
#include "defines.hpp"

namespace sph
{

    real W(real r, real h)
    {
        real q = r / h;
        if (q < 1.0)
        {
            return (1.0 / (M_PI * h * h * h)) * (1.0 - 1.5 * q * q + 0.75 * q * q * q);
        }
        else if (q < 2.0)
        {
            return (1.0 / (M_PI * h * h * h)) * 0.25 * std::pow(2.0 - q, 3.0);
        }
        else
        {
            return 0.0;
        }
    }

    void load_sedov_taylor(std::shared_ptr<sph::Simulation> sim,
                           std::shared_ptr<sph::SPHParameters> param)
    {
#if DIM == 3
        // 1) Simulation parameters
        int N = 50;                     // Particles per dimension
        real L = 2.0;                   // Box size (-1 to 1, so 2 m in each dimension, total volume 8 m³)
        real dx = L / N;                // Grid spacing ~0.0667 m
        real total_mass = 8.0;          // Total mass (density 1 kg/m³, volume 8 m³)
        real h = 0.1;                   // Smoothing length
        vec_t center = {0.0, 0.0, 0.0}; // Explosion center (adjusted to origin)
        real r_max = 2.0 * h;           // Kernel support radius ~0.2 m
        auto &p = sim->get_particles();
        p.clear();
        p.reserve(N * N * N);

        // 2) Place particles on a regular grid
        for (int i = 0; i < N; i++)
        {
            for (int j = 0; j < N; j++)
            {
                for (int k = 0; k < N; k++)
                {
                    vec_t r = {
                        -1.0 + (i + 0.5) * dx,
                        -1.0 + (j + 0.5) * dx,
                        -1.0 + (k + 0.5) * dx};

                    SPHParticle p_i;
                    p_i.pos = r;
                    p.emplace_back(p_i);
                }
            }
        }

        // 3) Assign particle properties
        const size_t total = p.size();
        const real mass = total_mass / total; // Mass per particle ~2.96296e-04 kg
        real sum_W = 0.0;

        // First pass: Compute sum of W(r, h) for particles within r_max to normalize energy
        for (size_t idx = 0; idx < total; ++idx)
        {
            auto &pp = p[idx];
            vec_t dr = pp.pos - center;
            real r = std::sqrt(dr[0] * dr[0] + dr[1] * dr[1] + dr[2] * dr[2]);
            if (r < r_max)
            {
                sum_W += W(r, h);
            }
        }

        // Ensure total energy is 1.0 J by setting the normalization constant C
        real total_energy = 1.0;                // Target total energy in joules
        real C = total_energy / (mass * sum_W); // Normalization constant for total energy 1 J

        // Second pass: Set all properties
        for (size_t idx = 0; idx < total; ++idx)
        {
            auto &pp = p[idx];
            vec_t dr = pp.pos - center;
            real r = std::sqrt(dr[0] * dr[0] + dr[1] * dr[1] + dr[2] * dr[2]);

            pp.vel = 0.0;   // Initial velocity
            pp.mass = mass; // Particle mass
            pp.dens = 1.0;  // Initial density
            pp.id = (int)idx;

            // Thermal energy assignment
            if (r < r_max)
            {
                pp.ene = C * W(r, h); // Energy within kernel support, scaled to total 1.0 J
            }
            else
            {
                pp.ene = 1.0e-6; // Small ambient energy (in J/kg)
            }
            pp.pres = (param->physics.gamma - 1.0) * pp.dens * pp.ene; // Pressure
            pp.sml = h;                                                // Set smoothing length to h
        }

        // 4) Finalize simulation
        sim->set_particle_num((int)p.size());
#endif
    }

    REGISTER_SAMPLE("sedov_taylor", load_sedov_taylor);

} // namespace sph