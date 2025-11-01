// shock_detection.cpp
#include "utilities/shock_detection.hpp"
#include "core/simulation.hpp"
#include "core/particle.hpp"
#include "utilities/periodic.hpp"
#include "kernel/kernel_function.hpp"
#include "utilities/defines.hpp"
#include "cmath"
#include <omp.h>
#include <algorithm>

namespace sph
{

    // Wendland-based one-dimensional kernel weight for shock detection.
    inline real W1D_wendland(real x, real h)
    {
        // Use absolute value of x.
        real q = std::abs(x) / h;
        if (q >= 1.0)
            return 0.0;
        // A Wendland-like form in 1D (the normalization constant here is chosen by tuning)
        return (std::pow(1.0 - q, 4) * (1.0 + 4.0 * q)) / h;
    }

    // Wendland-based two-dimensional kernel weight for shock detection.
    inline real W2D_wendland(real r, real h)
    {
        real q = r / h;
        if (q >= 1.0)
            return 0.0;
        // For 2D, we use the Wendland C4 kernel with sigma = 9/π:
        const real sigma = 9.0 / M_PI;
        return sigma * std::pow(1.0 - q, 6) * (1.0 + 6.0 * q + (35.0 / 3.0) * q * q) / (h * h);
    }

    // Main shock detection routine.
    // For each particle (excluding walls), this function computes a SPH-based pressure gradient,
    // forms a unit shock normal, and then using Wendland-based weighting functions,
    // computes weighted upstream and downstream averages of pressure, density, and velocity.
    // Finally, it estimates the Mach number using the pressure jump relation.
    void detect_shocks(std::shared_ptr<Simulation> sim,
                       const Periodic *periodic,
                       real gamma_val,
                       real h_factor /* typically 1.0 */)
    {
        auto &particles = sim->get_particles();
        int N = sim->get_particle_num();

#pragma omp parallel for
        for (int i = 0; i < N; ++i)
        {
            SPHParticle &pi = particles[i];
            if (pi.is_wall)
                continue;

            // Use an effective smoothing length.
            real h_i = pi.sml * h_factor;

            // --- Step 1: Compute Pressure Gradient ---
            // gradP_i = (1/ρ_i) * Σ_j m_j (P_j - P_i) ∇W(|r_i - r_j|, h_i)
            vec_t gradP = 0.0;
            for (int j = 0; j < N; ++j)
            {
                if (i == j)
                    continue;
                SPHParticle &pj = particles[j];
                vec_t r_ij = periodic->calc_r_ij(pi.pos, pj.pos);
                real r = std::abs(r_ij);
                // Here we use the kernel gradient from the simulation's kernel.
                vec_t gradW = sim->get_kernel()->dw(r_ij, r, h_i);
                gradP += gradW * (pj.pres - pi.pres) * pj.mass;
            }
            gradP /= pi.dens;
            real gradP_mag = std::abs(gradP);
            if (gradP_mag < 1e-6)
            {
                // If the pressure gradient is too small, set shock sensor to zero.
                pi.shockSensor = 0.0;
                continue;
            }

            // --- Step 2: Define the Shock Normal ---
            vec_t n = gradP / gradP_mag; // unit vector in the direction of gradP

            // --- Step 3: Classify Neighbors and Compute Weighted Averages ---
            real sumW_up = 0.0, sumW_down = 0.0;
            real P_up = 0.0, P_down = 0.0;
            real dens_up = 0.0, dens_down = 0.0;
            real v_up = 0.0, v_down = 0.0;

            for (int j = 0; j < N; ++j)
            {
                if (i == j)
                    continue;
                SPHParticle &pj = particles[j];
                vec_t r_ij = periodic->calc_r_ij(pi.pos, pj.pos);
                // Projection along the shock normal:
                real s_ij = inner_product(r_ij, n);
                // Perpendicular distance:
                vec_t r_parallel = n * s_ij;
                vec_t r_perp = r_ij - r_parallel;
                real d_perp = std::abs(r_perp);

                // Use Wendland-based weightings in 1D and 2D.
                if (s_ij < 0)
                { // Upstream side
                    real weight = W1D_wendland(-s_ij, h_i) * W2D_wendland(d_perp, h_i);
                    sumW_up += weight;
                    P_up += weight * pj.pres;
                    dens_up += weight * pj.dens;
                    v_up += weight * inner_product(pj.vel, n);
                }
                else if (s_ij > 0)
                { // Downstream side
                    real weight = W1D_wendland(s_ij, h_i) * W2D_wendland(d_perp, h_i);
                    sumW_down += weight;
                    P_down += weight * pj.pres;
                    dens_down += weight * pj.dens;
                    v_down += weight * inner_product(pj.vel, n);
                }
            }

            if (sumW_up > 0)
            {
                P_up /= sumW_up;
                dens_up /= sumW_up;
                v_up /= sumW_up;
            }
            if (sumW_down > 0)
            {
                P_down /= sumW_down;
                dens_down /= sumW_down;
                v_down /= sumW_down;
            }

            // --- Step 4: Estimate the Mach Number ---
            // Using the relation for an ideal gas:
            //    P_down/P_up = 1 + (2γ/(γ+1))(M^2 - 1)
            // Rearranged to yield:
            //    M = sqrt( 1 + ((P_down/P_up - 1)*(γ+1))/(2γ) )
            real Mach = 0.0;
            if (P_up > 0 && P_down > 0)
            {
                real ratio = P_down / P_up;
                Mach = std::sqrt(1.0 + ((ratio - 1.0) * (gamma_val + 1.0)) / (2.0 * gamma_val));
            }
            // Optionally, you might enforce a threshold so that only supersonic shocks (Mach > 1) are flagged.

            // --- Step 5: Store the Result ---
            pi.shockSensor = Mach;
        }
    }

} // namespace sph
