#include "defines.hpp"
#include "particle.hpp"
#include "periodic.hpp"
#include "simulation.hpp"
#include "bhtree.hpp"
#include "kernel/kernel_function.hpp"
#include "gdisph/gdi_fluid_force.hpp"

#ifdef EXHAUSTIVE_SEARCH
#include "exhaustive_search.hpp"
#endif

namespace sph
{
    namespace gdisph
    {

        void GodnouvFluidForce::initialize(std::shared_ptr<SPHParameters> param)
        {
            sph::FluidForce::initialize(param);
            m_is_2nd_order = param->gsph.is_2nd_order; // For potential future use
            m_gamma = param->physics.gamma;            // Adiabatic index from parameters
            m_use_balsara_switch = false;              // Explicitly disable for GDISPH Case 1
            hll_solver();                              // Initialize the HLL solver
        }

        void GodnouvFluidForce::calculation(std::shared_ptr<Simulation> sim)
        {
            auto &particles = sim->get_particles();
            auto *periodic = sim->get_periodic().get();
            const int num = sim->get_particle_num();
            auto *kernel = sim->get_kernel().get();
            auto *tree = sim->get_tree().get();

#pragma omp parallel for
            for (int i = 0; i < num; ++i)
            {
                auto &p_i = particles[i];
                std::vector<int> neighbor_list(m_neighbor_number * neighbor_list_size);

// Neighbor search
#ifdef EXHAUSTIVE_SEARCH
                int n_neighbor = exhaustive_search(
                    p_i, p_i.sml, particles, num, neighbor_list,
                    m_neighbor_number * neighbor_list_size, periodic, true);
#else
                int n_neighbor = tree->neighbor_search(p_i, neighbor_list, particles, true);
#endif

                // Precompute particle i's terms
                const vec_t &r_i = p_i.pos;
                const vec_t &v_i = p_i.vel;
                const real gamma2_u_i = sqr(m_gamma - 1.0) * p_i.ene;    // Energy-related term
                const real gamma2_u_per_pres_i = gamma2_u_i / p_i.pres;  // Pressure scaling
                const real f_i = 1.0 - p_i.gradh / (p_i.mass * p_i.ene); // Correction factor

                vec_t acc(0.0);  // Acceleration accumulator
                real dene = 0.0; // Energy change accumulator

                // Neighbor loop
                for (int n = 0; n < n_neighbor; ++n)
                {
                    const int j = neighbor_list[n];
                    auto &p_j = particles[j];
                    const vec_t r_ij = periodic->calc_r_ij(r_i, p_j.pos);
                    const real r = std::abs(r_ij);
                    if (r >= std::max(p_i.sml, p_j.sml) || r == 0.0)
                        continue;

                    // Kernel gradients
                    const vec_t dw_i = kernel->dw(r_ij, r, p_i.sml);
                    const vec_t dw_j = kernel->dw(r_ij, r, p_j.sml);
                    const vec_t v_ij = v_i - p_j.vel; // Actual velocity difference

                    // Particle j's correction factors
                    const real gamma2_u_j = sqr(m_gamma - 1.0) * p_j.ene;
                    const real gamma2_u_per_pres_j = gamma2_u_j / p_j.pres;
                    const real f_j = 1.0 - p_j.gradh / (p_j.mass * p_j.ene);

                    // Compute HLL states
                    real left[4], right[4];
                    compute_hll_states(p_i, p_j, r_ij, left, right);

                    // Solve Riemann problem using HLL solver
                    real pstar, vstar;
                    m_solver(left, right, pstar, vstar);

                    // Interface pressure (no artificial viscosity)
                    real p_star = pstar;

                    // Momentum update (symmetric form)
                    acc -= dw_i * (p_j.mass * (gamma2_u_per_pres_i * p_i.ene * f_i + 0.5 * p_star)) +
                           dw_j * (p_j.mass * (gamma2_u_per_pres_j * p_j.ene * f_j + 0.5 * p_star));

                    // Energy update using actual velocity difference
                    dene += p_j.mass * gamma2_u_per_pres_i * p_i.ene * f_i * inner_product(v_ij, dw_i);
                }

                p_i.acc = acc;
                p_i.dene = dene;
            }
        }

        // Helper function to compute left and right states for HLL solver
        void GodnouvFluidForce::compute_hll_states(const SPHParticle &p_i, const SPHParticle &p_j, const vec_t &r_ij, real left[], real right[])
        {
            const real r = std::abs(r_ij);
            vec_t n = r_ij / r; // Interface normal

            // Velocity components along the normal
            real u_l = inner_product(p_i.vel, n);
            real u_r = inner_product(p_j.vel, n);

            // Primitive variables
            real rho_l = p_i.dens;
            real rho_r = p_j.dens;
            real p_l = p_i.pres;
            real p_r = p_j.pres;
            real c_l = p_i.sound;
            real c_r = p_j.sound;

            left[0] = u_l;
            left[1] = rho_l;
            left[2] = p_l;
            left[3] = c_l;
            right[0] = u_r;
            right[1] = rho_r;
            right[2] = p_r;
            right[3] = c_r;
        }

        // Define the HLL Riemann solver
        void GodnouvFluidForce::hll_solver()
        {
            m_solver = [&](const real left[], const real right[], real &pstar, real &vstar)
            {
                // Extract state variables
                const real u_l = left[0], rho_l = left[1], p_l = left[2], c_l = left[3];
                const real u_r = right[0], rho_r = right[1], p_r = right[2], c_r = right[3];

                // Roe averages for wave speed estimation
                const real roe_l = std::sqrt(rho_l);
                const real roe_r = std::sqrt(rho_r);
                const real roe_inv = 1.0 / (roe_l + roe_r);
                const real u_t = (roe_l * u_l + roe_r * u_r) * roe_inv;
                const real c_t = (roe_l * c_l + roe_r * c_r) * roe_inv;

                // HLL wave speeds
                const real s_l = std::min(u_l - c_l, u_t - c_t); // Left wave speed
                const real s_r = std::max(u_r + c_r, u_t + c_t); // Right wave speed

                // Intermediate computations
                const real c1 = rho_l * (s_l - u_l);
                const real c2 = rho_r * (s_r - u_r);
                const real c3 = 1.0 / (s_r - s_l); // Note: assumes s_r > s_l
                const real c4 = p_l + rho_l * (s_l - u_l) * (vstar - u_l);
                const real c5 = p_r + rho_r * (s_r - u_r) * (vstar - u_r);

                // HLL star states
                vstar = (s_r * u_r - s_l * u_l + (p_l - p_r) / (rho_l * rho_r)) * c3;
                pstar = (s_r * p_l - s_l * p_r + rho_l * rho_r * (s_r - s_l) * (u_l - u_r)) * c3;
            };
        }

    } // namespace gdisph
} // namespace sph