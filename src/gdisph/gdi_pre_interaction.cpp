/* ================================
 * gdi_pre_interaction.cpp
 * Modified for anisotropic kernel support (GDISPH version)
 * ================================ */
#include <algorithm>
#include "parameters.hpp"
#include "gdisph/gdi_pre_interaction.hpp"
#include "simulation.hpp"
#include "periodic.hpp"
#include "openmp.hpp"
#include "kernel/kernel_function.hpp"
#include "exception.hpp"
#include "bhtree.hpp"

namespace sph
{
    namespace gdisph
    {
        real PreInteraction::newton_raphson(
            const SPHParticle &p_i,
            const std::vector<SPHParticle> &particles,
            const std::vector<int> &neighbor_list,
            const int n_neighbor,
            const Periodic *periodic,
            const KernelFunction *kernel)
        {
            // If not anisotropic, use base class implementation
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
                auto &particles = sim->get_particles();
                const int num = sim->get_particle_num();
                auto *periodic = sim->get_periodic().get();
                auto *kernel = sim->get_kernel().get();
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
                    if (i != j)
                    {
                        real r_local = std::abs(r_ij);
                        real v_sig = p_i.sound + p_j.sound - 3.0 * inner_product(r_ij, p_i.vel - p_j.vel) / (r_local + 1e-12);
                        if (v_sig > 0 && (p_i.sml / v_sig) < h_per_v_sig.get())
                        {
                            h_per_v_sig.get() = p_i.sml / v_sig;
                        }
                    }
                }
            }
            sim->set_h_per_v_sig(h_per_v_sig.min());
        }
    }
}
