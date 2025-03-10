#include "relaxation/lane_emden_relaxation.hpp"
#include <omp.h>
#include <algorithm>
#include "logger.hpp"
#include "exception.hpp"
#include "defines.hpp"

namespace sph
{
    void LaneEmdenRelaxation::add_relaxation_force(std::shared_ptr<Simulation> sim,
                                                   const SPHParameters &params)
    {
        // This merges old "add_relaxation_force", "compute_relaxation_force", etc.
        auto &particles = sim->get_particles();
        int num_p = sim->get_particle_num();

        // For polytropic index n=1.5 => gamma=5/3 => just define:
        const real n = 1.5;
        const real gamma_val = 5.0 / 3.0;
        // central surface density (could come from param or just 1.0)
        const real rho_c = 1.0;
        // polytropic constant
        const real K = 1.0;
        // radial scaling alpha = param->alpha_scaling
        const real alpha = (params.alpha_scaling != 0.0) ? params.alpha_scaling : 1.0;

#pragma omp parallel for
        for (int i = 0; i < num_p; i++)
        {
            if (particles[i].is_wall)
                continue;

            // position, radius
            const vec_t &pos = particles[i].pos;
            real r_phys = std::sqrt(inner_product(pos, pos));
            if (r_phys < 1e-12)
            {
                // no force at exact center
                continue;
            }
            real xi = r_phys / alpha;

            // get θ and derivative from LaneEmden data
            real theta_val = m_data.get_theta(xi);
            real dtheta = m_data.dtheta_dxi(xi);

            // compute radial acceleration
            // For n=1.5 => n * gamma_val = 2.5
            const real n_gamma = n * gamma_val; // = 2.5
            real prefactor = K * n_gamma * std::pow(rho_c, gamma_val - 1.0) / alpha;

            // a_r = - prefactor * dθ/dξ / θ
            if (theta_val < 1e-12)
                continue; // avoid division
            real a_r = -prefactor * dtheta;

            // radial unit vector
            vec_t e_r;
            for (int d = 0; d < DIM; d++)
            {
                e_r[d] = pos[d] / r_phys;
            }
            vec_t relax_acc;
            for (int d = 0; d < DIM; d++)
            {
                relax_acc[d] = a_r * e_r[d];
            }
            // Subtract it from the current acceleration (like in the old code).
            // Optionally zero velocity for pure relaxation, or user’s choice.
            particles[i].acc -= relax_acc;
            particles[i].vel = 0.0;
        }
        WRITE_LOG << "[LaneEmdenRelaxation] Applied radial Lane–Emden force to "
                  << num_p << " particles.";
    }

} // namespace sph
