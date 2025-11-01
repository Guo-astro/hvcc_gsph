/* ================================
 * d_pre_interaction.cpp
 * Modified for anisotropic 3D kernel support (DISPH version)
 * ================================ */
#include <algorithm>
#include <cmath>
#include "core/parameters.hpp"
#include "algorithms/disph/d_pre_interaction.hpp"
#include "core/simulation.hpp"
#include "utilities/periodic.hpp"
#include "utilities/openmp.hpp"
#include "kernel/kernel_function.hpp"
#include "utilities/exception.hpp"
#include "tree/bhtree.hpp"

namespace sph
{
    namespace disph
    {

        real PreInteraction::newton_raphson(
            const SPHParticle &p_i,
            const std::vector<SPHParticle> &particles,
            const std::vector<int> &neighbor_list,
            const int n_neighbor,
            const Periodic *periodic,
            const KernelFunction *kernel)
        {
            // Otherwise, delegate to the base class implementation.
            return sph::PreInteraction::newton_raphson(p_i, particles, neighbor_list, n_neighbor, periodic, kernel);
        }

        void PreInteraction::calculation(std::shared_ptr<Simulation> sim)
        {
            // Get the simulation timestep.
            const real dt = sim->get_dt();

            // First-call initialization: set initial smoothing lengths and neighbor counts.
            if (m_first)
            {
                auto &particles = sim->get_particles();
                const int num = sim->get_particle_num();
                auto *periodic = sim->get_periodic().get();
                for (int i = 0; i < num; ++i)
                {
                    auto &p_i = particles[i];
                    if (p_i.is_point_mass)
                    {
                        continue;
                    }

                    std::vector<int> neighbor_list(m_neighbor_number * neighbor_list_size);
                    int effectiveDim;
                    real A_eff;
                    if (m_anisotropic)
                    {
                        effectiveDim = 2;
                        A_eff = M_PI;
                        p_i.sml = std::pow(m_neighbor_number * p_i.mass / (p_i.dens * A_eff), 1.0 / 2.0) * m_kernel_ratio;
                    }
                    else
                    {
                        effectiveDim = m_twoAndHalf ? 2 : DIM;
                        A_eff = (effectiveDim == 1 ? 2.0 : (effectiveDim == 2 ? M_PI : 4.0 * M_PI / 3.0));
                        p_i.sml = std::pow(m_neighbor_number * p_i.mass / (p_i.dens * A_eff), 1.0 / effectiveDim) * m_kernel_ratio;
                    }
#ifdef EXHAUSTIVE_SEARCH
                    int n_neighbor = exhaustive_search(p_i, p_i.sml, particles, num,
                                                       neighbor_list, m_neighbor_number * neighbor_list_size,
                                                       periodic, false);
#else
                    int n_neighbor = sim->get_tree()->neighbor_search(p_i, neighbor_list, particles, false);
#endif
                    p_i.neighbor = n_neighbor;
                }
                m_first = false;
            }

            auto &particles = sim->get_particles();
            const int num = sim->get_particle_num();
            auto *periodic = sim->get_periodic().get();
            auto *kernel = sim->get_kernel().get();
            omp_real h_per_v_sig(std::numeric_limits<real>::max());

#pragma omp parallel for
            for (int i = 0; i < num; ++i)
            {
                auto &p_i = particles[i];
                if (p_i.is_point_mass)
                {
                    continue;
                }

                // Recompute smoothing length for each particle based on its density.
                std::vector<int> neighbor_list(m_neighbor_number * neighbor_list_size);
                int effectiveDim;
                real A_eff;
                if (m_anisotropic)
                {
                    effectiveDim = 2;
                    A_eff = M_PI;
                    p_i.sml = std::pow(m_neighbor_number * p_i.mass / (p_i.dens * A_eff), 1.0 / 2.0) * m_kernel_ratio;
                }
                else
                {
                    effectiveDim = m_twoAndHalf ? 2 : DIM;
                    A_eff = (effectiveDim == 1 ? 2.0 : (effectiveDim == 2 ? M_PI : 4.0 * M_PI / 3.0));
                    p_i.sml = std::pow(m_neighbor_number * p_i.mass / (p_i.dens * A_eff), 1.0 / effectiveDim) * m_kernel_ratio;
                }
#ifdef EXHAUSTIVE_SEARCH
                int n_neighbor = exhaustive_search(p_i, p_i.sml, particles, num,
                                                   neighbor_list, m_neighbor_number * neighbor_list_size,
                                                   periodic, false);
#else
                int n_neighbor = sim->get_tree()->neighbor_search(p_i, neighbor_list, particles, false);
#endif

                // Initialize accumulators for density and pressure calculations.
                real dens_i = 0.0;
                real pres_i = 0.0;
                real dh_pres_i = 0.0;
                real n_i = 0.0;
                real dh_n_i = 0.0;
                real v_sig_max = p_i.sound * 2.0;
                int valid_neighbors = 0;

                // Loop over neighbors to compute density, pressure, and kernel derivatives.
                for (int n = 0; n < n_neighbor; ++n)
                {
                    int j = neighbor_list[n];
                    auto &p_j = particles[j];
                    if (p_j.is_point_mass)
                        continue; // Skip point masses

                    vec_t r_ij = periodic->calc_r_ij(p_i.pos, p_j.pos);
                    real r = std::abs(r_ij);
                    bool valid = false;
                    if (m_anisotropic)
                    {
                        // Compute an effective anisotropic distance.
                        real r_xy = std::sqrt(r_ij[0] * r_ij[0] + r_ij[1] * r_ij[1]);
                        real r_aniso = std::sqrt((r_xy / p_i.sml) * (r_xy / p_i.sml) +
                                                 (r_ij[2] / m_hz) * (r_ij[2] / m_hz));
                        if (r_aniso < 1.0)
                            valid = true;
                    }
                    else
                    {
                        if (r < p_i.sml)
                            valid = true;
                    }
                    if (!valid)
                        continue;

                    valid_neighbors++;
                    const real w_ij = kernel->w(r, p_i.sml);
                    const real dhw_ij = kernel->dhw(r, p_i.sml);
                    dens_i += p_j.mass * w_ij;
                    n_i += w_ij;
                    pres_i += p_j.mass * p_j.ene * w_ij;
                    dh_pres_i += p_j.mass * p_j.ene * dhw_ij;
                    dh_n_i += dhw_ij;

                    if (i != j)
                    {
                        const real v_sig = p_i.sound + p_j.sound - 3.0 * inner_product(r_ij, p_i.vel - p_j.vel) / (r + 1e-12);
                        if (v_sig > v_sig_max)
                        {
                            v_sig_max = v_sig;
                        }
                    }
                }

                p_i.neighbor = valid_neighbors;
                p_i.dens = dens_i;
                p_i.pres = (m_gamma - 1.0) * pres_i;
                p_i.gradh = p_i.sml / (effectiveDim * n_i) * dh_pres_i / (1.0 + p_i.sml / (effectiveDim * n_i) * dh_n_i);
                
                // DISPH: Calculate volume element V = m/ρ
                p_i.volume = p_i.mass / p_i.dens;

                const real h_per_v_sig_i = p_i.sml / v_sig_max;
                if (h_per_v_sig.get() > h_per_v_sig_i)
                {
                    h_per_v_sig.get() = h_per_v_sig_i;
                }

                // Artificial viscosity.
                if (m_use_balsara_switch && DIM != 1)
                {
#if DIM != 1
                    real div_v = 0.0;
#if DIM == 2
                    real rot_v = 0.0;
#else
                    vec_t rot_v = 0.0;
#endif
                    // Loop over neighbors to compute velocity divergence and rotation.
                    for (int n = 0; n < n_neighbor; ++n)
                    {
                        int j = neighbor_list[n];
                        auto &p_j = particles[j];
                        if (p_j.is_point_mass)
                            continue;

                        vec_t r_ij = periodic->calc_r_ij(p_i.pos, p_j.pos);
                        real r = std::abs(r_ij);
                        bool valid = false;
                        if (m_anisotropic)
                        {
                            real r_xy = std::sqrt(r_ij[0] * r_ij[0] + r_ij[1] * r_ij[1]);
                            real r_aniso = std::sqrt((r_xy / p_i.sml) * (r_xy / p_i.sml) +
                                                     (r_ij[2] / m_hz) * (r_ij[2] / m_hz));
                            if (r_aniso < 1.0)
                                valid = true;
                        }
                        else
                        {
                            if (r < p_i.sml)
                                valid = true;
                        }
                        if (!valid)
                            continue;

                        const vec_t dw = kernel->dw(r_ij, r, p_i.sml);
                        const vec_t v_ij = p_i.vel - p_j.vel;
                        div_v -= p_j.mass * p_j.ene * inner_product(v_ij, dw);
                        rot_v += vector_product(v_ij, dw) * (p_j.mass * p_j.ene);
                    }
                    const real p_inv = (m_gamma - 1.0) / p_i.pres;
                    div_v *= p_inv;
                    rot_v *= p_inv;
                    p_i.balsara = std::abs(div_v) / (std::abs(div_v) + std::abs(rot_v) + 1e-4 * p_i.sound / p_i.sml);

                    // Time-dependent viscosity parameter.
                    if (m_use_time_dependent_av)
                    {
                        const real tau_inv = m_epsilon * p_i.sound / p_i.sml;
                        const real dalpha = (-(p_i.alpha - m_alpha_min) * tau_inv +
                                             std::max(-div_v, (real)0.0) * (m_alpha_max - p_i.alpha)) *
                                            dt;
                        p_i.alpha += dalpha;
                    }
#endif
                }
                else if (m_use_time_dependent_av)
                {
                    real div_v = 0.0;
                    for (int n = 0; n < n_neighbor; ++n)
                    {
                        int j = neighbor_list[n];
                        auto &p_j = particles[j];
                        if (p_j.is_point_mass)
                            continue;

                        vec_t r_ij = periodic->calc_r_ij(p_i.pos, p_j.pos);
                        real r = std::abs(r_ij);
                        bool valid = false;
                        if (m_anisotropic)
                        {
                            real r_xy = std::sqrt(r_ij[0] * r_ij[0] + r_ij[1] * r_ij[1]);
                            real r_aniso = std::sqrt((r_xy / p_i.sml) * (r_xy / p_i.sml) +
                                                     (r_ij[2] / m_hz) * (r_ij[2] / m_hz));
                            if (r_aniso < 1.0)
                                valid = true;
                        }
                        else
                        {
                            if (r < p_i.sml)
                                valid = true;
                        }
                        if (!valid)
                            continue;

                        const vec_t dw = kernel->dw(r_ij, r, p_i.sml);
                        const vec_t v_ij = p_i.vel - p_j.vel;
                        div_v -= p_j.mass * p_j.ene * inner_product(v_ij, dw);
                    }
                    const real p_inv = (m_gamma - 1.0) / p_i.pres;
                    div_v *= p_inv;
                    const real tau_inv = m_epsilon * p_i.sound / p_i.sml;
                    const real s_i = std::max(-div_v, (real)0.0);
                    p_i.alpha = (p_i.alpha + dt * tau_inv * m_alpha_min + s_i * dt * m_alpha_max) /
                                (1.0 + dt * tau_inv + s_i * dt);
                }
            }

            sim->set_h_per_v_sig(h_per_v_sig.min());
        }

    } // namespace disph
} // namespace sph
