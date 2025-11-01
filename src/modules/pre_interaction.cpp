#include <algorithm>
#include "modules/pre_interaction.hpp"
#include "core/simulation.hpp"
#include "core/parameters.hpp"
#include "core/logger.hpp"
#include "utilities/periodic.hpp"
#include "utilities/openmp.hpp"
#include "kernel/kernel_function.hpp"
#include "utilities/exception.hpp"
#include "tree/bhtree.hpp"

namespace sph
{
    real PreInteraction::newton_raphson(
        const SPHParticle &p_i,
        const std::vector<SPHParticle> &particles,
        const std::vector<int> &neighbor_list,
        const int n_neighbor,
        const Periodic *periodic,
        const KernelFunction *kernel)
    {
        const int kd = kernelDim(m_twoAndHalf);
        real h_i = p_i.sml / m_kernel_ratio;
        constexpr real A = DIM == 1 ? 2.0 : DIM == 2 ? M_PI
                                                     : 4.0 * M_PI / 3.0;
        const real b = p_i.mass * m_neighbor_number / A;

        // f = rho h^d - b
        // f' = drho/dh h^d + d rho h^{d-1}

        constexpr real epsilon = 1e-4;
        constexpr int max_iter = 10;
        const auto &r_i = p_i.pos;
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

            const real f = dens * powh_dim(h_i, kd) - b;
            const real df = ddens * powh_dim(h_i, kd) + DIM * dens * powh_(h_i, kd);

            h_i -= f / df;

            if (std::abs(h_i - h_b) < (h_i + h_b) * epsilon)
            {
                return h_i;
            }
        }

#pragma omp critical
        {
            WRITE_LOG << "Particle id " << p_i.id << " is not convergence";
        }

        return p_i.sml / m_kernel_ratio;
    }

    void PreInteraction::initialize(std::shared_ptr<SPHParameters> param)
    {
        m_twoAndHalf = param->two_and_half_sim; // effective 2D kernel if true
        m_use_time_dependent_av = param->av.use_time_dependent_av;
        m_hz = param->h_z;
        WRITE_LOG << "Anistropy h_z, m_hz: " << m_hz;
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

    // A helper function for initial smoothing.
    void PreInteraction::perform_initial_smoothing(std::shared_ptr<Simulation> sim, bool twoAndHalf, real kernel_ratio, int neighbor_number, real gamma)
    {
        auto &particles = sim->get_particles();
        int num = sim->get_particle_num();
        auto *periodic = sim->get_periodic().get();
        auto *kernel = sim->get_kernel().get();
        auto *tree = sim->get_tree().get();

#pragma omp parallel for
        for (int i = 0; i < num; ++i)
        {
            auto &p_i = particles[i];
            std::vector<int> neighbor_list(neighbor_number * neighbor_list_size);
            int effectiveDim = twoAndHalf ? 2 : DIM;
            real A_eff = (effectiveDim == 1 ? 2.0 : (effectiveDim == 2 ? M_PI : 4.0 * M_PI / 3.0));
            p_i.sml = std::pow(neighbor_number * p_i.mass / (p_i.dens * A_eff), 1.0 / effectiveDim) * kernel_ratio;
#ifdef EXHAUSTIVE_SEARCH
            int n_neighbor = exhaustive_search(p_i, p_i.sml, particles, num, neighbor_list, neighbor_number * neighbor_list_size, periodic, false);
#else
            int n_neighbor = tree->neighbor_search(p_i, neighbor_list, particles, false);
#endif

            real dens_i = 0.0;
            real dh_dens_i = 0.0;
            for (int n = 0; n < n_neighbor; ++n)
            {
                int j = neighbor_list[n];
                auto &p_j = particles[j];
                vec_t r_ij = periodic->calc_r_ij(p_i.pos, p_j.pos);
                real r = std::abs(r_ij);
                if (r >= p_i.sml)
                {
                    continue;
                }
                dens_i += p_j.mass * kernel->w(r, p_i.sml);
                dh_dens_i += p_j.mass * kernel->dhw(r, p_i.sml);
            }
            p_i.dens = dens_i;
            p_i.pres = (gamma - 1.0) * dens_i * p_i.ene;
            p_i.gradh = 1.0 / (1.0 + p_i.sml / (effectiveDim * dens_i) * dh_dens_i);
        }
    }

    void PreInteraction::calculation(std::shared_ptr<Simulation> sim)
    {
        // Perform initial smoothing only once
        if (m_first)
        {
            perform_initial_smoothing(sim, m_twoAndHalf, m_kernel_ratio, m_neighbor_number, m_gamma);
            m_first = false;
        }

        auto &particles = sim->get_particles();
        int num = sim->get_particle_num();
        auto *periodic = sim->get_periodic().get();
        auto *kernel = sim->get_kernel().get();
        auto *tree = sim->get_tree().get();

        const real dt = sim->get_dt();
        // We want the global minimum h/v_sig
        omp_real h_per_v_sig(std::numeric_limits<real>::max());

#pragma omp parallel for
        for (int i = 0; i < num; ++i)
        {
            auto &p_i = particles[i];
            // Skip point-mass (e.g. boundary) particles
            if (p_i.is_point_mass)
                continue;

            // Pick effective dimension if 'twoAndHalf' is on
            int effectiveDim = m_twoAndHalf ? 2 : DIM;
            real A_eff = (effectiveDim == 1 ? 2.0
                                            : (effectiveDim == 2 ? M_PI
                                                                 : 4.0 * M_PI / 3.0));

            // 1) Estimate smoothing length (initial guess)
            p_i.sml = std::pow(m_neighbor_number * p_i.mass / (p_i.dens * A_eff),
                               1.0 / effectiveDim) *
                      m_kernel_ratio;

            // 2) Get neighbor list
            std::vector<int> neighbor_list(m_neighbor_number * neighbor_list_size);
#ifdef EXHAUSTIVE_SEARCH
            int n_neighbor = exhaustive_search(p_i, p_i.sml, particles, num,
                                               neighbor_list,
                                               m_neighbor_number * neighbor_list_size,
                                               periodic, false);
#else
            int n_neighbor = tree->neighbor_search(p_i, neighbor_list,
                                                   particles, false);
#endif

            // 3) If iterative smoothing is requested, refine h via Newton-Raphson
            if (m_iteration)
            {
                p_i.sml = newton_raphson(p_i, particles, neighbor_list,
                                         n_neighbor, periodic, kernel);
            }

            // 4) Compute density, derivative of density, and track v_sig
            real dens_i = 0.0;
            real dh_dens_i = 0.0;
            real v_sig_max = p_i.sound * 2.0;
            int neighborCount = 0;

            for (int n = 0; n < n_neighbor; ++n)
            {
                int j = neighbor_list[n];
                auto &p_j = particles[j];
                if (p_j.is_point_mass)
                    continue; // Exclude point masses from SPH density

                // Distance
                vec_t r_ij = periodic->calc_r_ij(p_i.pos, p_j.pos);
                real r = std::abs(r_ij);
                if (r >= p_i.sml)
                    continue;

                // Accumulate density & its derivative
                dens_i += p_j.mass * kernel->w(r, p_i.sml);
                dh_dens_i += p_j.mass * kernel->dhw(r, p_i.sml);
                neighborCount++;

                // Track maximum velocity signal (v_sig)
                if (i != j)
                {
                    real v_sig = (p_i.sound + p_j.sound) - 3.0 * inner_product(r_ij, (p_i.vel - p_j.vel)) / (r + 1e-12);
                    if (v_sig > v_sig_max)
                    {
                        v_sig_max = v_sig;
                    }
                }
            }

            // 5) Update particle’s density, pressure, gradient correction
            p_i.dens = dens_i;
            p_i.pres = (m_gamma - 1.0) * dens_i * p_i.ene;
            p_i.gradh = 1.0 / (1.0 + (p_i.sml / (effectiveDim * dens_i + 1e-12)) * dh_dens_i);
            p_i.neighbor = neighborCount;

            // 6) Update global minimum of (h / v_sig)
            real h_vs_i = p_i.sml / (v_sig_max + 1e-12);
            if (h_vs_i < h_per_v_sig.get())
            {
                h_per_v_sig.get() = h_vs_i;
            }

            // 7) Artificial viscosity (Balsara switch / time-dependent α)
#if DIM != 1
            // Only do Balsara in 2D or 3D
            if (m_use_balsara_switch && effectiveDim != 1)
            {
                real div_v = 0.0;
#if DIM == 2
                real rot_v = 0.0; // 2D “z-component” of curl
#else
                vec_t rot_v(0.0); // 3D vector for curl
#endif
                // Re-loop neighbors to compute div(v) and curl(v)
                for (int n = 0; n < n_neighbor; ++n)
                {
                    int j = neighbor_list[n];
                    auto &p_j = particles[j];
                    if (p_j.is_point_mass)
                        continue;

                    vec_t r_ij = periodic->calc_r_ij(p_i.pos, p_j.pos);
                    real r = std::abs(r_ij);
                    if (r >= p_i.sml)
                        continue;

                    vec_t v_ij = p_i.vel - p_j.vel;
                    vec_t dw = kernel->dw(r_ij, r, p_i.sml);

                    div_v -= p_j.mass * inner_product(v_ij, dw);
#if DIM == 2
                    rot_v += p_j.mass * (v_ij[0] * dw[1] - v_ij[1] * dw[0]);
#else
                    rot_v += vector_product(v_ij, dw) * p_j.mass * p_j.mass;
#endif
                }
                div_v /= (p_i.dens + 1e-12);

#if DIM == 2
                rot_v /= (p_i.dens + 1e-12);
                real abs_rot_v = std::abs(rot_v);
#else
                real abs_rot_v = std::abs(rot_v) / (p_i.dens + 1e-12);
#endif

                // Balsara switch: alpha *= |divV| / (|divV| + |curlV| + small)
                p_i.balsara = std::abs(div_v) / (std::abs(div_v) + abs_rot_v + 1e-4 * p_i.sound / (p_i.sml + 1e-12));

                // (Optional) Time-dependent α
                if (m_use_time_dependent_av)
                {
                    real tau_inv = m_epsilon * p_i.sound / (p_i.sml + 1e-12);
                    real s_i = std::max(-div_v, (real)0.0); // compress only

                    // dα/dt = –(α–α_min)*τ⁻¹ + max(-divV, 0)*(α_max – α)
                    real alpha_old = p_i.alpha;
                    real dalpha = (-(alpha_old - m_alpha_min) * tau_inv + s_i * (m_alpha_max - alpha_old)) * dt;

                    p_i.alpha = alpha_old + dalpha;
                    // Optionally clamp
                    if (p_i.alpha < m_alpha_min)
                        p_i.alpha = m_alpha_min;
                    if (p_i.alpha > m_alpha_max)
                        p_i.alpha = m_alpha_max;
                }
            }
            else if (m_use_time_dependent_av)
            {
                // No Balsara switch path
                // Need div(v) only
                real div_v = 0.0;
                for (int n = 0; n < n_neighbor; ++n)
                {
                    int j = neighbor_list[n];
                    auto &p_j = particles[j];
                    if (p_j.is_point_mass)
                        continue;

                    vec_t r_ij = periodic->calc_r_ij(p_i.pos, p_j.pos);
                    real r = std::abs(r_ij);
                    if (r >= p_i.sml)
                        continue;

                    vec_t v_ij = p_i.vel - p_j.vel;
                    vec_t dw = kernel->dw(r_ij, r, p_i.sml);
                    div_v -= p_j.mass * inner_product(v_ij, dw);
                }
                div_v /= (p_i.dens + 1e-12);

                real tau_inv = m_epsilon * p_i.sound / (p_i.sml + 1e-12);
                real s_i = std::max(-div_v, (real)0.0);
                real alpha_old = p_i.alpha;
                real dalpha = (-(alpha_old - m_alpha_min) * tau_inv + s_i * (m_alpha_max - alpha_old)) * dt;

                p_i.alpha = alpha_old + dalpha;
                if (p_i.alpha < m_alpha_min)
                    p_i.alpha = m_alpha_min;
                if (p_i.alpha > m_alpha_max)
                    p_i.alpha = m_alpha_max;
            }
#endif // DIM != 1
        } // end of parallel loop

        // Update global h_per_v_sig
        sim->set_h_per_v_sig(h_per_v_sig.min());

#ifndef EXHAUSTIVE_SEARCH
        // Rebuild or reset tree kernel data if needed
        tree->set_kernel();
#endif
    }
} // namespace sph
