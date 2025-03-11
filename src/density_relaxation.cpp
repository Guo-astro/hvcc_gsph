#pragma once

#include "density_relaxation.hpp"
#include "simulation.hpp"
#include "parameters.hpp"
#include "logger.hpp"

#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <cmath>
#include <omp.h>

namespace sph
{

    // ---------------------------------------------------------------------
    // compute_relaxation_force: Adds an artificial radial acceleration that
    // drives each particle toward the adiabatic (γ=5/3) razor-thin disk
    // polytropic solution: a_r = -(1/Σ) dP/dr.
    //
    // For n=1.5 => γ=5/3 => P=K Σ^(5/3),  Σ=ρ_c θ^n,  and the derivative
    // simplifies to a_r = -C * dθ/dξ, where C = [K n γ ρ_c^(γ)] / [α ρ_c].
    // ---------------------------------------------------------------------
    vec_t compute_relaxation_force(const SPHParticle &p, const SPHParameters &params)
    {
        // polytropic / disk parameters
        const real n = 1.5;                      // polytropic index => gamma=5/3
        const real gamma_val = 5.0 / 3.0;        // adiabatic index
        const real rho_c = 1.0;                  // central surface density
        const real K = 1.0;                      // polytropic constant
        const real alpha = params.alpha_scaling; // r = alpha * xi

        // radial coordinate
        vec_t force{};
        real r_phys = std::sqrt(inner_product(p.pos, p.pos));
        if (r_phys < 1e-12)
            return force; // no force at center

        // dimensionless coordinate
        real xi = r_phys / alpha;

        // get θ(ξ) and its derivative
        real theta_val = getTheta(xi);
        real dtheta = dTheta_dXi(xi);

        // For n=1.5 => n * gamma_val = 2.5
        const real n_gamma = n * gamma_val; // = 2.5
        // prefactor: C = [K * n_gamma * ρ_c^γ] / [alpha * ρ_c]
        // because ρ_c^γ / ρ_c = ρ_c^(γ-1) => for γ=5/3 => ρ_c^(2/3).
        // => C = K * 2.5 * ρ_c^(2/3) / alpha
        real prefactor = K * n_gamma * std::pow(rho_c, gamma_val - 1.0) / alpha;

        // radial acceleration: a_r = - C * dθ/dξ
        real a_r = -prefactor * dtheta;

        // radial unit vector
        vec_t e_r;
        for (int i = 0; i < DIM; i++)
        {
            e_r[i] = p.pos[i] / r_phys;
        }
        // final force = m * a_r, but here we just store acceleration
        for (int i = 0; i < DIM; i++)
        {
            force[i] = a_r * e_r[i];
        }
        return force;
    }

    // ---------------------------------------------------------------------
    // add_relaxation_force: Loop over fluid particles, compute the artificial
    // radial acceleration, and add it to their accelerations. Optionally
    // zero out velocity for a pure "relaxation" approach.
    // ---------------------------------------------------------------------
    void add_relaxation_force(std::shared_ptr<Simulation> sim, const SPHParameters &params)
    {
        auto &particles = sim->get_particles();
        int num_p = sim->get_particle_num();

#pragma omp parallel for
        for (int i = 0; i < num_p; i++)
        {
            if (particles[i].is_wall)
                continue;
            // compute the radial "relaxation" acceleration
            vec_t relax_acc = compute_relaxation_force(particles[i], params);

            // subtract from the current acceleration to push toward equilibrium
            particles[i].acc += relax_acc;

            // optionally zero out velocity for pure relaxation
            particles[i].vel = 0.0;
        }
        WRITE_LOG << "[density_relaxation] Added Lane–Emden-based relaxation force to particles.";
    }
} // namespace sph
