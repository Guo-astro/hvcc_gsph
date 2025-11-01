#include "utilities/defines.hpp"
#include "algorithms/disph/d_fluid_force.hpp"
#include "core/particle.hpp"
#include "utilities/periodic.hpp"
#include "core/simulation.hpp"
#include "tree/bhtree.hpp"
#include "kernel/kernel_function.hpp"

#ifdef EXHAUSTIVE_SEARCH
#include "tree/exhaustive_search.hpp"
#endif

namespace sph
{
    namespace disph
    {

        void FluidForce::initialize(std::shared_ptr<SPHParameters> param)
        {
            sph::FluidForce::initialize(param);
            m_gamma = param->physics.gamma;
        }

        // DISPH Formulation from Yuasa & Mori (2023) - arXiv:2312.03224
        // Uses volume elements V = m/ρ instead of density directly
        // Pressure force: dv/dt = -Σ m_j (P_i/V_i² + P_j/V_j²) ∇W
        // Energy: du/dt = Σ m_j (P_i/V_i² + P_j/V_j²) v_ij · ∇W
        void FluidForce::calculation(std::shared_ptr<Simulation> sim)
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
                if (p_i.is_point_mass)
                {
                    continue;
                }

                if (p_i.is_wall)
                {
                    p_i.acc = 0.0;
                    p_i.dene = 0.0;
                    continue;
                }

                std::vector<int> neighbor_list(m_neighbor_number * neighbor_list_size);

                // neighbor search
#ifdef EXHAUSTIVE_SEARCH
                int const n_neighbor = exhaustive_search(p_i, p_i.sml, particles, num, neighbor_list, m_neighbor_number * neighbor_list_size, periodic, true);
#else
                int const n_neighbor = tree->neighbor_search(p_i, neighbor_list, particles, true);
#endif

                // Get particle i properties  
                const vec_t &r_i = p_i.pos;
                const vec_t &v_i = p_i.vel;
                const real h_i = p_i.sml;
                const real u_i = p_i.ene;  // Specific internal energy
                const real q_i = p_i.pres / (m_gamma - 1.0);  // Internal energy density: q = P/(γ-1)
                const real U_i = p_i.mass * u_i;  // Total internal energy
                const real gradh_i = p_i.gradh;  // grad-h correction term

                vec_t acc(0.0);
                real dene = 0.0;

                for (int n = 0; n < n_neighbor; ++n)
                {
                    int const j = neighbor_list[n];
                    auto &p_j = particles[j];
                    if (p_j.is_point_mass)
                    {
                        continue;
                    }

                    const vec_t r_ij = periodic->calc_r_ij(r_i, p_j.pos);
                    const real r = std::abs(r_ij);

                    if (r >= std::max(h_i, p_j.sml) || r == 0.0)
                    {
                        continue;
                    }

                    // Get kernel gradient
                    const vec_t dw_i = kernel->dw(r_ij, r, h_i);
                    const vec_t dw_j = kernel->dw(r_ij, r, p_j.sml);
                    
                    // Get particle j properties
                    const real u_j = p_j.ene;  // Specific internal energy
                    const real q_j = p_j.pres / (m_gamma - 1.0);  // Internal energy density  
                    const real U_j = p_j.mass * u_j;  // Total internal energy
                    const real gradh_j = p_j.gradh;  // grad-h correction term
                    
                    // Velocity difference for energy
                    const vec_t v_ij = v_i - p_j.vel;
                    
                    // DISPH momentum equation (38):
                    // dv_i/dt = -(γ-1) * Σ_j u_i*U_j * [gradh_i*∇W_ij(h_i)/q_i + gradh_j*∇W_ij(h_j)/q_j]
                    const real coef = (m_gamma - 1.0) * u_i * U_j;
                    const real term_i = coef * gradh_i / q_i;
                    const real term_j = coef * gradh_j / q_j;
                    acc -= dw_i * term_i + dw_j * term_j;
                    
                    // DISPH energy equation (39):
                    // dU_i/dt = (γ-1) * gradh_i * Σ_j (U_i*U_j/q_i) * v_ij · ∇W_ij(h_i)
                    // Convert to specific internal energy: du_i/dt = dU_i/dt / m_i
                    const real ene_coef = (m_gamma - 1.0) * gradh_i * U_i * U_j / (q_i * p_i.mass);
                    dene += ene_coef * inner_product(v_ij, dw_i);
                    
                    // Artificial viscosity (if enabled)
                    const real pi_ij = artificial_viscosity(p_i, p_j, r_ij);
                    if (pi_ij != 0.0)
                    {
                        const vec_t dw_ij = (dw_i + dw_j) * 0.5;
                        acc -= dw_ij * (p_j.mass * 0.5 * pi_ij);
                        dene += p_j.mass * 0.5 * pi_ij * inner_product(v_ij, dw_ij);
                    }
                    
                    // Artificial conductivity (if enabled)
                    if (m_use_ac)
                    {
                        const vec_t dw_ij = (dw_i + dw_j) * 0.5;
                        const real dene_ac = artificial_conductivity(p_i, p_j, r_ij, dw_ij);
                        if (dene_ac != 0.0)
                        {
                            dene += dene_ac;
                        }
                    }
                }

                p_i.acc = acc;
                p_i.dene = dene;
            }
        }
    }
}