#include "relaxation/lane_emden_relaxation.hpp"
#include <omp.h>
#include <algorithm>
#include "logger.hpp"
#include "exception.hpp"
#include "defines.hpp"

namespace sph
{

    vec_t LaneEmdenRelaxation::compute_relaxation_force(const SPHParticle &p, const SPHParameters &params)
    {
        // Polytropic / disk parameters
        const real n = 1.5;                      // Polytropic index => gamma = 5/3
        const real gamma_val = 5.0 / 3.0;        // Adiabatic index
        const real rho_c = 1.0;                  // Central surface density
        const real K = 1.0;                      // Polytropic constant
        const real alpha = params.alpha_scaling; // r = alpha * xi
        const real eps = 1e-12;                  // Small value to avoid singularities
        const real h_z = params.h_z;

        // Cylindrical radial coordinate and vertical position
        real r_cyl = std::sqrt(p.pos[0] * p.pos[0] + p.pos[1] * p.pos[1]);
        real z = p.pos[2];
        vec_t force{};

        // Dimensionless coordinate (use xi = 0 on the z-axis)
        real xi = (r_cyl > eps) ? r_cyl / alpha : 0.0;

        // Get θ(ξ) and its derivative using LaneEmdenData
        real theta_val = m_data.get_theta(xi);
        real dtheta = (r_cyl > eps) ? m_data.dtheta_dxi(xi) : 0.0; // Radial force is zero at r_cyl = 0

        // Check for valid theta_val (assuming theta should be non-negative)
        if (theta_val < 0.0)
            theta_val = 0.0; // Fallback to avoid unphysical results

        // Compute prefactor: C = K * n * gamma * rho_c^(gamma-1) / alpha
        const real n_gamma = n * gamma_val; // 1.5 * 5/3 = 2.5
        real prefactor = K * n_gamma * std::pow(rho_c, gamma_val - 1.0) / alpha;

        // Radial acceleration: a_r = -C * dθ/dξ
        real a_r = -prefactor * dtheta;

        // Surface density: Sigma = rho_c * theta^(1.5)
        real Sigma = rho_c * std::pow(theta_val, 1.5);

        // Vertical acceleration: a_z = -(z / h_z^2) * (K * gamma * Sigma^(gamma-1))
        real a_z = -(z / (h_z * h_z)) * (K * gamma_val * std::pow(Sigma, gamma_val - 1.0));

        // Apply forces
        if (r_cyl > eps)
        {
            // Cylindrical radial unit vector components
            real e_r_cyl_x = p.pos[0] / r_cyl;
            real e_r_cyl_y = p.pos[1] / r_cyl;
            force[0] = a_r * e_r_cyl_x;
            force[1] = a_r * e_r_cyl_y;
        }
        else
        {
            // On the z-axis, radial force is zero
            force[0] = 0.0;
            force[1] = 0.0;
        }
        // Vertical force is applied directly
        force[2] = a_z;

        return force;
    }

    void LaneEmdenRelaxation::add_relaxation_force(std::shared_ptr<Simulation> sim,
                                                   const SPHParameters &params)
    {
        // Get particle data from the simulation
        auto &particles = sim->get_particles();
        int num_p = sim->get_particle_num();

        // Parallel loop over all particles
#pragma omp parallel for
        for (int i = 0; i < num_p; i++)
        {
            if (particles[i].is_wall)
                continue; // Skip wall particles

            // Compute relaxation acceleration for the particle
            vec_t relax_acc = compute_relaxation_force(particles[i], params);

            // Subtract relaxation acceleration from current acceleration
            for (int d = 0; d < 3; d++)
            {
                particles[i].acc[d] -= relax_acc[d];
            }

            // Zero the particle's velocity for relaxation
            for (int d = 0; d < 3; d++)
            {
                particles[i].vel[d] = 0.0;
            }
        }

        // Log the operation
        WRITE_LOG << "[LaneEmdenRelaxation] Applied radial and vertical Lane–Emden acceleration to "
                  << num_p << " particles.";
    }

} // namespace sph