#include <algorithm>
#include "pre_interaction.hpp"
#include "simulation.hpp"
#include "parameters.hpp"
#include "logger.hpp"
#include "periodic.hpp"
#include "openmp.hpp"
#include "kernel/kernel_function.hpp"
#include "exception.hpp"
#include "bhtree.hpp"

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
        // Initial guess for smoothing length h
        real h = p_i.sml;
        const real h_min = 1e-6; // Minimum h to prevent division by zero
        const real tolerance = 1e-6;
        const int max_iterations = 20;

        // Target neighbor number
        const real N_desired = static_cast<real>(m_neighbor_number);

        for (int iter = 0; iter < max_iterations; ++iter)
        {
            // Compute N(h): weighted sum of neighbors within kernel support
            real N_h = 0.0;
            real dN_dh = 0.0;

            for (int n = 0; n < n_neighbor; ++n)
            {
                int j = neighbor_list[n];
                const SPHParticle &p_j = particles[j];
                vec_t r_ij = periodic->calc_r_ij(p_i.pos, p_j.pos);
                real r = std::abs(r_ij);
                real q = r / h;

                // Kernel value and derivative
                real W = kernel->W(q);
                real dW_dq = kernel->dW_dq(q);

                // Contribution to N(h)
                N_h += p_j.mass / p_j.dens * W;

                // Contribution to dN/dh
                // dW/dh = dW/dq * dq/dh = dW/dq * (-r/h^2)
                dN_dh += p_j.mass / p_j.dens * dW_dq * (-r / (h * h));
            }

            // Residual and derivative
            real residual = N_h - N_desired;
            if (std::abs(residual) < tolerance)
            {
                break;
            }

            // Newton-Raphson update
            if (std::abs(dN_dh) < 1e-12)
            {
                throw std::runtime_error("newton_raphson: derivative too small, cannot converge");
            }
            real dh = -residual / dN_dh;
            h += dh;

            // Ensure h remains positive
            if (h < h_min)
            {
                h = h_min;
            }
        }

        return h;
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
    static void perform_initial_smoothing(std::shared_ptr<Simulation> sim, bool twoAndHalf, real kernel_ratio, int neighbor_number, real gamma)
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
        if (m_first)
        {
            perform_initial_smoothing(sim, m_twoAndHalf, m_kernel_ratio, m_neighbor_number, m_gamma);
            m_first = false;
        }
        auto &particles = sim->get_particles();
        int num = sim->get_particle_num();
        auto *periodic = sim->get_periodic().get();
        auto *kernel = sim->get_kernel().get();
        const real dt = sim->get_dt(); // Note: dt is not used in this routine.
        omp_real h_per_v_sig(std::numeric_limits<real>::max());

#pragma omp parallel for
        for (int i = 0; i < num; ++i)
        {
            auto &p_i = particles[i];
            std::vector<int> neighbor_list(m_neighbor_number * neighbor_list_size);
            int effectiveDim = m_twoAndHalf ? 2 : DIM;
            real A_eff = (effectiveDim == 1 ? 2.0 : (effectiveDim == 2 ? M_PI : 4.0 * M_PI / 3.0));
            p_i.sml = std::pow(m_neighbor_number * p_i.mass / (p_i.dens * A_eff), 1.0 / effectiveDim) * m_kernel_ratio;
#ifdef EXHAUSTIVE_SEARCH
            int n_neighbor = exhaustive_search(p_i, p_i.sml, particles, num, neighbor_list, m_neighbor_number * neighbor_list_size, periodic, false);
#else
            int n_neighbor = sim->get_tree()->neighbor_search(p_i, neighbor_list, particles, false);
#endif

            for (int n = 0; n < n_neighbor; ++n)
            {
                int j = neighbor_list[n];
                auto &p_j = particles[j];
                vec_t r_ij = periodic->calc_r_ij(p_i.pos, p_j.pos);
                real r = std::abs(r_ij);
                if (r >= p_i.sml)
                    continue;
                if (i != j)
                {
                    real r_local = r;
                    real v_sig = p_i.sound + p_j.sound - 3.0 * inner_product(r_ij, p_i.vel - p_j.vel) / (r_local + 1e-12);
                    if (v_sig > 0 && (p_i.sml / v_sig) < h_per_v_sig.get())
                        h_per_v_sig.get() = p_i.sml / v_sig;
                }
            }
        }
        sim->set_h_per_v_sig(h_per_v_sig.min());
    }

} // namespace sph
