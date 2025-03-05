#include <algorithm>

#include "parameters.hpp"
#include "gsph/g_pre_interaction.hpp"
#include "simulation.hpp"
#include "periodic.hpp"
#include "openmp.hpp"
#include "kernel/kernel_function.hpp"
#include "exception.hpp"
#include "bhtree.hpp"

#ifdef EXHAUSTIVE_SEARCH
#include "exhaustive_search.hpp"
#endif

namespace sph
{
    namespace gsph
    {

        void PreInteraction::initialize(std::shared_ptr<SPHParameters> param)
        {
            sph::PreInteraction::initialize(param);
            m_is_2nd_order = param->gsph.is_2nd_order;
        }

        void PreInteraction::calculation(std::shared_ptr<Simulation> sim)
        {
            if (m_first)
            {
                initial_smoothing(sim);
                m_first = false;
            }

            auto &particles = sim->get_particles();
            auto *periodic = sim->get_periodic().get();
            const int num = sim->get_particle_num();
            auto *kernel = sim->get_kernel().get();
            auto *tree = sim->get_tree().get();
            const real dt = sim->get_dt();

            omp_real h_per_v_sig(std::numeric_limits<real>::max());

            // For MUSCL and divergence check
            auto &grad_d = sim->get_vector_array("grad_density");
            auto &grad_p = sim->get_vector_array("grad_pressure");
            vec_t *grad_v[DIM] = {
                sim->get_vector_array("grad_velocity_0").data(),
#if DIM == 2
                sim->get_vector_array("grad_velocity_1").data(),
#elif DIM == 3
                sim->get_vector_array("grad_velocity_1").data(),
                sim->get_vector_array("grad_velocity_2").data(),
#endif
            };

#pragma omp parallel for
            for (int i = 0; i < num; ++i)
            {
                auto &p_i = particles[i];

                std::vector<int> neighbor_list(m_neighbor_number * neighbor_list_size);

                // Guess smoothing length
                constexpr real A = DIM == 1 ? 2.0 : DIM == 2 ? M_PI
                                                             : 4.0 * M_PI / 3.0;
                p_i.sml = std::pow(m_neighbor_number * p_i.mass / (p_i.dens * A), 1.0 / DIM) * m_kernel_ratio;

                // Neighbor search
#ifdef EXHAUSTIVE_SEARCH
                const int n_neighbor_tmp = exhaustive_search(p_i, p_i.sml, particles, num, neighbor_list, m_neighbor_number * neighbor_list_size, periodic, false);
#else
                const int n_neighbor_tmp = tree->neighbor_search(p_i, neighbor_list, particles, false);
#endif
                // Smoothing length
                if (m_iteration)
                {
                    p_i.sml = newton_raphson(p_i, particles, neighbor_list, n_neighbor_tmp, periodic, kernel);
                }

                // Density and related quantities
                real dens_i = 0.0;
                real dh_dens_i = 0.0;
                real pres_i = 0.0;    // DISPH pressure accumulator
                real dh_pres_i = 0.0; // DISPH gradh term
                real n_i = 0.0;       // DISPH neighbor count
                real dh_n_i = 0.0;    // DISPH gradh derivative
                real v_sig_max = p_i.sound * 2.0;
                int n_neighbor = 0;
                for (int n = 0; n < n_neighbor_tmp; ++n)
                {
                    int const j = neighbor_list[n];
                    auto &p_j = particles[j];
                    const vec_t r_ij = periodic->calc_r_ij(p_i.pos, p_j.pos);
                    const real r = std::abs(r_ij);

                    if (r >= p_i.sml)
                    {
                        continue;
                    }

                    ++n_neighbor;
                    const real w_ij = kernel->w(r, p_i.sml);
                    const real dhw_ij = kernel->dhw(r, p_i.sml);

                    // GSPH computations
                    dens_i += p_j.mass * w_ij;
                    dh_dens_i += p_j.mass * dhw_ij;

                    // DISPH computations
                    pres_i += p_j.mass * p_j.ene * w_ij;
                    dh_pres_i += p_j.mass * p_j.ene * dhw_ij;
                    n_i += w_ij;
                    dh_n_i += dhw_ij;

                    if (i != j)
                    {
                        const real v_sig = p_i.sound + p_j.sound - 3.0 * inner_product(r_ij, p_i.vel - p_j.vel) / r;
                        if (v_sig > v_sig_max)
                        {
                            v_sig_max = v_sig;
                        }
                    }
                }

                p_i.dens = dens_i;

                // Divergence condition from g_fluid_force.cpp
                if (p_i.switch_to_no_shock_region)
                {
                    // DISPH-style properties
                    p_i.pres = (m_gamma - 1.0) * pres_i;
                    p_i.gradh = p_i.sml / (DIM * n_i) * dh_pres_i / (1.0 + p_i.sml / (DIM * n_i) * dh_n_i);
                    // Artificial viscosity
                    if (m_use_balsara_switch && DIM != 1)
                    {
#if DIM != 1
                        // balsara switch
                        real div_v = 0.0;
#if DIM == 2
                        real rot_v = 0.0;
#else
                        vec_t rot_v = 0.0;
#endif
                        for (int n = 0; n < n_neighbor; ++n)
                        {
                            int const j = neighbor_list[n];
                            auto &p_j = particles[j];
                            const vec_t r_ij = periodic->calc_r_ij(p_i.pos, p_j.pos);
                            const real r = std::abs(r_ij);
                            const vec_t dw = kernel->dw(r_ij, r, p_i.sml);
                            const vec_t v_ij = p_i.vel - p_j.vel;
                            div_v -= p_j.mass * p_j.ene * inner_product(v_ij, dw);
                            rot_v += vector_product(v_ij, dw) * (p_j.mass * p_j.ene);
                        }
                        const real p_inv = (m_gamma - 1.0) / p_i.pres;
                        div_v *= p_inv;
                        rot_v *= p_inv;
                        p_i.balsara = std::abs(div_v) / (std::abs(div_v) + std::abs(rot_v) + 1e-4 * p_i.sound / p_i.sml);
                        // time dependent alpha
                        if (m_use_time_dependent_av)
                        {
                            const real tau_inv = m_epsilon * p_i.sound / p_i.sml;
                            const real dalpha = (-(p_i.alpha - m_alpha_min) * tau_inv + std::max(-div_v, (real)0.0) * (m_alpha_max - p_i.alpha)) * dt;
                            p_i.alpha += dalpha;
                        }

#endif
                    }
                    else if (m_use_time_dependent_av)
                    {
                        real div_v = 0.0;
                        for (int n = 0; n < n_neighbor; ++n)
                        {
                            int const j = neighbor_list[n];
                            auto &p_j = particles[j];
                            const vec_t r_ij = periodic->calc_r_ij(p_i.pos, p_j.pos);
                            const real r = std::abs(r_ij);
                            const vec_t dw = kernel->dw(r_ij, r, p_i.sml);
                            const vec_t v_ij = p_i.vel - p_j.vel;
                            div_v -= p_j.mass * p_j.ene * inner_product(v_ij, dw);
                        }
                        const real p_inv = (m_gamma - 1.0) / p_i.pres;
                        div_v *= p_inv;
                        const real tau_inv = m_epsilon * p_i.sound / p_i.sml;
                        const real s_i = std::max(-div_v, (real)0.0);
                        p_i.alpha = (p_i.alpha + dt * tau_inv * m_alpha_min + s_i * dt * m_alpha_max) / (1.0 + dt * tau_inv + s_i * dt);
                    }
                }
                else
                {
                    // GSPH defaults
                    p_i.pres = (m_gamma - 1.0) * dens_i * p_i.ene;
                    p_i.gradh = 1.0 / (1.0 + p_i.sml / (DIM * dens_i) * dh_dens_i);
                }

                p_i.neighbor = n_neighbor;

                const real h_per_v_sig_i = p_i.sml / v_sig_max;
                if (h_per_v_sig.get() > h_per_v_sig_i)
                {
                    h_per_v_sig.get() = h_per_v_sig_i;
                }
            }

            sim->set_h_per_v_sig(h_per_v_sig.min());

#ifndef EXHAUSTIVE_SEARCH
            tree->set_kernel();
#endif
        }

    } // namespace gsph
} // namespace sph