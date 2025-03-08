// src/density_relaxation.cpp
#include "density_relaxation.hpp"
#include "simulation.hpp"
#include "parameters.hpp"
#include "logger.hpp"
#include "lane_emden.hpp" // to use getTheta() and the laneEmden_x/table

#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <cmath>
#include <omp.h>

namespace sph
{
    // Helper: compute dθ/dξ at a given ξ using the Lane–Emden lookup table.
    real dTheta_dXi(real xi)
    {
        // Ensure the table is loaded.
        if (laneEmden_x.empty())
            loadLaneEmdenTableFromCSV(); // uses default filename

        if (xi <= laneEmden_x.front())
        {
            if (laneEmden_x.size() >= 2)
                return (laneEmden_theta[1] - laneEmden_theta[0]) / (laneEmden_x[1] - laneEmden_x[0]);
            else
                return 0.0;
        }
        if (xi >= laneEmden_x.back())
        {
            size_t n = laneEmden_x.size();
            return (laneEmden_theta[n - 1] - laneEmden_theta[n - 2]) / (laneEmden_x[n - 1] - laneEmden_x[n - 2]);
        }
        size_t i = 0;
        for (; i < laneEmden_x.size() - 1; ++i)
        {
            if (xi < laneEmden_x[i + 1])
                break;
        }
        return (laneEmden_theta[i + 1] - laneEmden_theta[i]) / (laneEmden_x[i + 1] - laneEmden_x[i]);
    }

    // compute_relaxation_force computes the extra acceleration based on the pressure gradient
    // derived from the Lane–Emden solution. Here we assume a polytropic equation of state:
    //
    //   P_target = K * (rho_c)^gamma * theta^(n+1)    with gamma = 1+1/n
    //
    // so that:
    //
    //   dP_target/dξ = K * (rho_c)^gamma * (n+1)* theta^n * (dθ/dξ)
    //   dP_target/dr  = (1/α)* dP_target/dξ      (since r = αξ)
    //
    // Then the acceleration correction is:
    //
    //   a_r = - (1/ρ) * (dP_target/dr)
    //
    // and we add that in the radial direction.
    vec_t compute_relaxation_force(const SPHParticle &p, const SPHParameters &params)
    {
        vec_t force{}; // Initialize all components to zero

        // --- Model parameters (adjust these as needed) ---
        const real n = 1.5;                   // Polytropic index
        const real gamma_val = 1.0 + 1.0 / n; // For n=1.5, gamma=5/3
        const real rho_c = 1.0;               // Central density (set from your problem)
        const real K = 1.0;                   // Polytropic constant (set from your problem)
        const real alpha_scaling = 1.0;       // Scale factor, so that r = α ξ

        // --- Compute radial coordinate r ---
        real r_phys = std::sqrt(inner_product(p.pos, p.pos));
        // Avoid division by zero for particles at center.
        if (r_phys < 1e-12)
            return force;

        // Compute the dimensionless coordinate ξ.
        real xi = r_phys / alpha_scaling;

        // Get θ(ξ) from the lookup table.
        real theta_val = getTheta(xi);
        // Compute dθ/dξ.
        real dtheta = dTheta_dXi(xi);

        // Compute dP/dξ = K * (rho_c)^gamma * (n+1)* theta^n * (dθ/dξ)
        real dP_dxi = K * std::pow(rho_c, gamma_val) * (n + 1.0) * std::pow(theta_val, n) * dtheta;
        // Then, dP/dr = dP/dξ / α.
        real dP_dr = dP_dxi / alpha_scaling;

        // Compute the relaxation acceleration: a_r = -(1/ρ) * dP/dr.
        real a_r = -(1.0 / p.dens) * dP_dr;

        // Compute the radial unit vector e_r = p.pos / |p.pos|.
        vec_t e_r;
        for (int i = 0; i < DIM; ++i)
        {
            e_r[i] = p.pos[i] / r_phys;
        }

        // Set the relaxation force vector: a_r * e_r.
        for (int i = 0; i < DIM; ++i)
        {
            force[i] = a_r * e_r[i];
        }
        return force;
    }

    // Loop over all (non-wall) particles and add the computed relaxation acceleration
    // (from the Lane–Emden pressure gradient) to each particle’s acceleration.
    void add_relaxation_force(std::shared_ptr<Simulation> sim, const SPHParameters &params)
    {
        auto &particles = sim->get_particles();
        int num_p = sim->get_particle_num();
#pragma omp parallel for
        for (int i = 0; i < num_p; ++i)
        {
            if (particles[i].is_wall)
                continue;
            vec_t relax_force = compute_relaxation_force(particles[i], params);
            particles[i].acc += relax_force;
        }
        WRITE_LOG << "Added relaxation force (derived from Lane–Emden pressure gradient) to particle accelerations.";
    }
} // namespace sph
