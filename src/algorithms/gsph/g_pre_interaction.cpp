/* ================================
 * g_pre_interaction.cpp
 * Modified for anisotropic kernel support (GSPH version)
 * ================================ */
#include <algorithm>
#include "core/parameters.hpp"
#include "algorithms/gsph/g_pre_interaction.hpp"
#include "core/simulation.hpp"
#include "utilities/periodic.hpp"
#include "utilities/openmp.hpp"
#include "kernel/kernel_function.hpp"
#include "utilities/exception.hpp"
#include "tree/bhtree.hpp"

#ifdef EXHAUSTIVE_SEARCH
#include "tree/exhaustive_search.hpp"
#endif
namespace sph
{
    namespace gsph
    {
        real PreInteraction::newton_raphson(
            const SPHParticle &p_i,
            const std::vector<SPHParticle> &particles,
            const std::vector<int> &neighbor_list,
            const int n_neighbor,
            const Periodic *periodic,
            const KernelFunction *kernel)
        {
            return sph::PreInteraction::newton_raphson(p_i, particles, neighbor_list, n_neighbor, periodic, kernel);
        }
        void PreInteraction::initialize(std::shared_ptr<SPHParameters> param)
        {
            // Call base PreInteraction::initialize
            sph::PreInteraction::initialize(param);
            m_is_2nd_order = param->gsph.is_2nd_order;
            m_anisotropic = param->anisotropic;
            if (m_anisotropic)
            {
                m_hz = param->h_z;
            }
        }

        void PreInteraction::calculation(std::shared_ptr<Simulation> sim)
        {
            if (m_first)
            {
                perform_initial_smoothing(sim, m_twoAndHalf, m_kernel_ratio, m_neighbor_number, m_gamma);
                m_first = false;
            }
            auto &particles = sim->get_particles();
            auto *periodic = sim->get_periodic().get();
            const int num = sim->get_particle_num();
            auto *kernel = sim->get_kernel().get();
            auto *tree = sim->get_tree().get();

            omp_real h_per_v_sig(std::numeric_limits<real>::max());
            // for MUSCL
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
                int n_neighbor_tmp = 0;

                // density etc.
                real dens_i = 0.0;
                real v_sig_max = p_i.sound * 2.0;
                const vec_t &pos_i = p_i.pos;
                for (int n = 0; n < n_neighbor; ++n)
                {
                    int j = neighbor_list[n];
                    auto &p_j = particles[j];
                    if (p_j.is_point_mass)
                    {
                        continue;
                    }
                    vec_t r_ij = periodic->calc_r_ij(p_i.pos, p_j.pos);
                    real r = std::abs(r_ij);
                    if (m_anisotropic)
                    {
                        real r_xy = std::sqrt(r_ij[0] * r_ij[0] + r_ij[1] * r_ij[1]);
                        real r_aniso = std::sqrt((r_xy / p_i.sml) * (r_xy / p_i.sml) +
                                                 (r_ij[2] / m_hz) * (r_ij[2] / m_hz));
                        if (r_aniso >= 1.0)
                            continue;
                    }
                    else
                    {
                        if (r >= p_i.sml)
                            continue;
                    }
                    ++n_neighbor_tmp;
                    dens_i += p_j.mass * kernel->w(r, p_i.sml);
                    if (i != j)
                    {
                        real r_local = std::abs(r_ij);
                        real v_sig = p_i.sound + p_j.sound - 3.0 * inner_product(r_ij, p_i.vel - p_j.vel) / (r_local + 1e-12);
                        if (v_sig > v_sig_max)
                        {
                            v_sig_max = v_sig;
                        }
                    }
                }
                p_i.dens = dens_i;
                p_i.pres = (m_gamma - 1.0) * dens_i * p_i.ene;
                p_i.neighbor = n_neighbor_tmp;
                const real h_per_v_sig_i = p_i.sml / v_sig_max;
                if (h_per_v_sig.get() > h_per_v_sig_i)
                {
                    h_per_v_sig.get() = h_per_v_sig_i;
                }

                // MUSCL法のための勾配計算
                if (!m_is_2nd_order)
                {
                    continue;
                }

                vec_t dd, du; // dP = (gamma - 1) * (rho * du + drho * u)
                vec_t dv[DIM];
                for (int n = 0; n < n_neighbor; ++n)
                {
                    int const j = neighbor_list[n];
                    auto &p_j = particles[j];
                    const vec_t r_ij = periodic->calc_r_ij(pos_i, p_j.pos);
                    const real r = std::abs(r_ij);
                    const vec_t dw_ij = kernel->dw(r_ij, r, p_i.sml);
                    dd += dw_ij * p_j.mass;
                    du += dw_ij * (p_j.mass * (p_j.ene - p_i.ene));
                    for (int k = 0; k < DIM; ++k)
                    {
                        dv[k] += dw_ij * (p_j.mass * (p_j.vel[k] - p_i.vel[k]));
                    }
                }
                grad_d[i] = dd;
                grad_p[i] = (dd * p_i.ene + du) * (m_gamma - 1.0);
                const real rho_inv = 1.0 / p_i.dens;
                for (int k = 0; k < DIM; ++k)
                {
                    grad_v[k][i] = dv[k] * rho_inv;
                }
            }

            sim->set_h_per_v_sig(h_per_v_sig.min());
#ifndef EXHAUSTIVE_SEARCH
            tree->set_kernel();
#endif
        }
    }
}
