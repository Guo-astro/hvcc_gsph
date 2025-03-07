#include "simulation.hpp"
#include "parameters.hpp"
#include <vector>
#include <cmath>
#include <algorithm>
#include <omp.h>
#include <logger.hpp>

// For PreInteraction dependency
#include "pre_interaction.hpp"
#include "bhtree.hpp"
#include "kernel/kernel_function.hpp"
#include "periodic.hpp"
#include "density_relaxation.hpp"

namespace sph
{
    // Changed to shared_ptr and fixed '¶ms' typo
    void perform_density_relaxation(std::shared_ptr<Simulation> sim, const SPHParameters &params)
    {
        // Check if density relaxation is enabled
        if (!params.density_relaxation.is_valid)
        {
            WRITE_LOG << "Density relaxation disabled.";
            return;
        }

        // Access particle data and parameters
        auto &particles = sim->get_particles();
        const int num_particles = sim->get_particle_num();
        const real damping = params.density_relaxation.damping_factor;
        const real tolerance = params.density_relaxation.tolerance;
        const int max_iter = params.density_relaxation.max_iterations;

        // Log start of density relaxation
        WRITE_LOG << "Starting density relaxation with max_iter=" << max_iter
                  << ", tolerance=" << tolerance << ", damping=" << damping;

        // Store initial densities as target values
        std::vector<real> target_dens(num_particles);
#pragma omp parallel for
        for (int i = 0; i < num_particles; ++i)
        {
            target_dens[i] = particles[i].dens;
        }

        // Iterative relaxation process
        int iter = 0;
        real max_error = 0.0;
        do
        {
            // Update densities using a temporary PreInteraction object
            // Since Simulation doesn’t have update_densities(), simulate it
            PreInteraction pre;
            pre.initialize(std::make_shared<SPHParameters>(params));
            pre.calculation(sim);

            // Compute density errors and displacements
            max_error = 0.0;
#pragma omp parallel for reduction(max : max_error)
            for (int i = 0; i < num_particles; ++i)
            {
                if (particles[i].is_wall)
                    continue; // Skip wall particles

                // Calculate density error
                real rho = particles[i].dens;
                real rho_target = target_dens[i];
                real error = (rho - rho_target) / rho_target;
                max_error = std::max(max_error, std::abs(error));

                // Compute displacement based on density gradient
                vec_t disp(0.0);
                std::vector<int> neighbor_list(params.physics.neighbor_number * neighbor_list_size);
                int n_neighbor = sim->get_tree()->neighbor_search(particles[i], neighbor_list, particles, false);
                const vec_t &r_i = particles[i].pos;
                real h_i = particles[i].sml;

                for (int n = 0; n < n_neighbor; ++n)
                {
                    int j = neighbor_list[n];
                    const vec_t r_ij = sim->get_periodic()->calc_r_ij(r_i, particles[j].pos);
                    real r = std::abs(r_ij);
                    if (r >= h_i || r == 0.0)
                        continue;

                    real w = sim->get_kernel()->w(r, h_i);
                    disp += r_ij * (particles[j].mass * w / (rho * rho_target));
                }

                // Update position with damping and apply periodic boundaries
                particles[i].pos -= disp * damping;
                sim->get_periodic()->apply(particles[i].pos);
            }

            // Log progress
            WRITE_LOG << "Relaxation iter " << iter + 1 << ", max density error = " << max_error;
            iter++;
        } while (iter < max_iter && max_error > tolerance);

        // Perform final density update
        PreInteraction pre_final;
        pre_final.initialize(std::make_shared<SPHParameters>(params));
        pre_final.calculation(sim);

        // Log completion
        WRITE_LOG << "Density relaxation completed after " << iter << " iterations, final max error = " << max_error;
    }

} // namespace sph