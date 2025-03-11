/* ================================
 * pre_interaction.cpp
 * ================================ */
#include <algorithm>
#include "parameters.hpp"
#include "pre_interaction.hpp"
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

    // Added member m_twoAndHalf to store the simulation’s 2.5D flag.
    void PreInteraction::initialize(std::shared_ptr<SPHParameters> param)
    {
        m_twoAndHalf = param->two_and_half_sim; // NEW: effective 2D kernel if true

        m_use_time_dependent_av = param->av.use_time_dependent_av;
        if (m_use_time_dependent_av)
        {
            m_alpha_max = param->av.alpha_max;
            m_alpha_min = param->av.alpha_min;
            m_epsilon = param->av.epsilon;
        }
        m_use_balsara_switch = param->av.use_balsara_switch;
        m_gamma = param->physics.gamma;
        m_neighbor_number = param->physics.neighbor_number;
        m_iteration = param->iterative_sml;
        if (m_iteration)
        {
            m_kernel_ratio = 1.2;
        }
        else
        {
            m_kernel_ratio = 1.0;
        }
        m_first = true;
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
        const real dt = sim->get_dt();
        auto *tree = sim->get_tree().get();

        omp_real h_per_v_sig(std::numeric_limits<real>::max());

#pragma omp parallel for
        for (int i = 0; i < num; ++i)
        {
            auto &p_i = particles[i];
            std::vector<int> neighbor_list(m_neighbor_number * neighbor_list_size);

            // Use effective kernel dimension: if two_and_half_sim is true, use 2; otherwise use DIM.
            int effectiveDim = m_twoAndHalf ? 2 : DIM;
            real A_eff = (effectiveDim == 1 ? 2.0 : (effectiveDim == 2 ? M_PI : 4.0 * M_PI / 3.0));
            p_i.sml = std::pow(m_neighbor_number * p_i.mass / (p_i.dens * A_eff), 1.0 / effectiveDim) * m_kernel_ratio;

            // neighbor search remains in full 3D
#ifdef EXHAUSTIVE_SEARCH
            const int n_neighbor_tmp = exhaustive_search(p_i, p_i.sml, particles, num, neighbor_list,
                                                         m_neighbor_number * neighbor_list_size, periodic, false);
#else
            const int n_neighbor_tmp = tree->neighbor_search(p_i, neighbor_list, particles, false);
#endif

            // If iterative smoothing is enabled, update the smoothing length.
            if (m_iteration)
            {
                p_i.sml = newton_raphson(p_i, particles, neighbor_list, n_neighbor_tmp, periodic, kernel);
            }

            // Now compute density and related quantities.
            real dens_i = 0.0;
            real dh_dens_i = 0.0;
            real v_sig_max = p_i.sound * 2.0;
            const vec_t &pos_i = p_i.pos;
            int n_neighbor = 0;
            for (int n = 0; n < n_neighbor_tmp; ++n)
            {
                int const j = neighbor_list[n];
                auto &p_j = particles[j];
                const vec_t r_ij = periodic->calc_r_ij(pos_i, p_j.pos);
                const real r = std::abs(r_ij);

                if (r >= p_i.sml)
                {
                    continue; // Skip neighbors outside smoothing length.
                }

                ++n_neighbor;
                dens_i += p_j.mass * kernel->w(r, p_i.sml);
                dh_dens_i += p_j.mass * kernel->dhw(r, p_i.sml);

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
            p_i.pres = (m_gamma - 1.0) * dens_i * p_i.ene;
            // Use effectiveDim in the grad-h term as well.
            p_i.gradh = 1.0 / (1.0 + p_i.sml / (effectiveDim * dens_i) * dh_dens_i);
            p_i.neighbor = n_neighbor;

            const real h_per_v_sig_i = p_i.sml / v_sig_max;
            if (h_per_v_sig.get() > h_per_v_sig_i)
            {
                h_per_v_sig.get() = h_per_v_sig_i;
            }

            // Artificial viscosity (unchanged; still computed using full DIM values)
            if (m_use_balsara_switch && DIM != 1)
            {
#if DIM != 1
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
                    const vec_t r_ij = periodic->calc_r_ij(pos_i, p_j.pos);
                    const real r = std::abs(r_ij);
                    const vec_t dw = kernel->dw(r_ij, r, p_i.sml);
                    const vec_t v_ij = p_i.vel - p_j.vel;
                    div_v -= p_j.mass * inner_product(v_ij, dw);
                    rot_v += vector_product(v_ij, dw) * p_j.mass;
                }
                div_v /= p_i.dens;
                rot_v /= p_i.dens;
                p_i.balsara = std::abs(div_v) / (std::abs(div_v) + std::abs(rot_v) + 1e-4 * p_i.sound / p_i.sml);

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
                    int const j = neighbor_list[n];
                    auto &p_j = particles[j];
                    const vec_t r_ij = periodic->calc_r_ij(pos_i, p_j.pos);
                    const real r = std::abs(r_ij);
                    const vec_t dw = kernel->dw(r_ij, r, p_i.sml);
                    const vec_t v_ij = p_i.vel - p_j.vel;
                    div_v -= p_j.mass * inner_product(v_ij, dw);
                }
                div_v /= p_i.dens;
                const real tau_inv = m_epsilon * p_i.sound / p_i.sml;
                const real s_i = std::max(-div_v, (real)0.0);
                p_i.alpha = (p_i.alpha + dt * tau_inv * m_alpha_min + s_i * dt * m_alpha_max) / (1.0 + dt * tau_inv + s_i * dt);
            }
        }

        sim->set_h_per_v_sig(h_per_v_sig.min());

#ifndef EXHAUSTIVE_SEARCH
        tree->set_kernel();
#endif
    }

    void PreInteraction::initial_smoothing(std::shared_ptr<Simulation> sim)
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
            const vec_t &pos_i = p_i.pos;
            std::vector<int> neighbor_list(m_neighbor_number * neighbor_list_size);

            // Use effective kernel dimension for initial smoothing as well.
            int effectiveDim = m_twoAndHalf ? 2 : DIM;
            real A_eff = (effectiveDim == 1 ? 2.0 : (effectiveDim == 2 ? M_PI : 4.0 * M_PI / 3.0));
            p_i.sml = std::pow(m_neighbor_number * p_i.mass / (p_i.dens * A_eff), 1.0 / effectiveDim);

            // neighbor search
#ifdef EXHAUSTIVE_SEARCH
            int const n_neighbor = exhaustive_search(p_i, p_i.sml, particles, num, neighbor_list,
                                                     m_neighbor_number * neighbor_list_size, periodic, false);
#else
            int const n_neighbor = tree->neighbor_search(p_i, neighbor_list, particles, false);
#endif

            // Compute density by summing contributions from neighbors.
            real dens_i = 0.0;
            for (int n = 0; n < n_neighbor; ++n)
            {
                int const j = neighbor_list[n];
                auto &p_j = particles[j];
                const vec_t r_ij = periodic->calc_r_ij(pos_i, p_j.pos);
                const real r = std::abs(r_ij);

                if (r >= p_i.sml)
                {
                    break;
                }

                dens_i += p_j.mass * kernel->w(r, p_i.sml);
            }

            p_i.dens = dens_i;
        }
    }

    // In the Newton–Raphson routine, we also use the effective kernel dimension.
    real PreInteraction::newton_raphson(
        const SPHParticle &p_i,
        const std::vector<SPHParticle> &particles,
        const std::vector<int> &neighbor_list,
        const int n_neighbor,
        const Periodic *periodic,
        const KernelFunction *kernel)
    {
        // Use m_kernel_ratio to scale the initial guess.
        real h_i = p_i.sml / m_kernel_ratio;
        int effectiveDim = m_twoAndHalf ? 2 : DIM;
        real A_eff = (effectiveDim == 1 ? 2.0 : (effectiveDim == 2 ? M_PI : 4.0 * M_PI / 3.0));
        const real b = p_i.mass * m_neighbor_number / A_eff;

        // f = rho * h^(effectiveDim) - b
        // f' = (drho/dh) * h^(effectiveDim) + effectiveDim * rho * h^(effectiveDim-1)
        constexpr real epsilon = 1e-4;
        constexpr int max_iter = 10;
        const auto &r_i = p_i.pos;

        // Define a lambda to compute h^(effectiveDim) for our Newton iteration.
        auto powh_eff = [effectiveDim](real h) -> real
        {
            if (effectiveDim == 1)
                return h;
            else if (effectiveDim == 2)
                return h; // for 2D, our kernel normalization uses h^1 (see above)
            else
                return h * h;
        };

        for (int i = 0; i < max_iter; ++i)
        {
            const real h_b = h_i;

            real dens = 0.0;
            real ddens = 0.0;
            for (int n = 0; n < n_neighbor; ++n)
            {
                int const j = neighbor_list[n];
                auto &p_j = particles[j];
                const vec_t r_ij = periodic->calc_r_ij(r_i, p_j.pos);
                const real r = std::abs(r_ij);

                if (r >= h_i)
                {
                    break;
                }

                dens += p_j.mass * kernel->w(r, h_i);
                ddens += p_j.mass * kernel->dhw(r, h_i);
            }

            const real f = dens * powh_dim(h_i, effectiveDim) - b;
            const real df = ddens * powh_dim(h_i, effectiveDim) + effectiveDim * dens * powh_eff(h_i);

            h_i -= f / df;

            if (std::abs(h_i - h_b) < (h_i + h_b) * epsilon)
            {
                return h_i;
            }
        }

#pragma omp critical
        {
            WRITE_LOG << "Particle id " << p_i.id << " did not converge";
        }

        return p_i.sml / m_kernel_ratio;
    }

} // namespace sph
