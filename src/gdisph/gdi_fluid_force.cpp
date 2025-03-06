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
#include <logger.hpp>

namespace sph
{
    namespace gdisph
    {

        void FluidForce::initialize(std::shared_ptr<SPHParameters> param)
        {
            sph::FluidForce::initialize(param);
            m_is_2nd_order = param->gsph.is_2nd_order;
            m_gamma = param->physics.gamma;

            hll_solver();
        }

        // van Leer (1979) limiter
        inline real limiter(const real dq1, const real dq2)
        {
            const real dq1dq2 = dq1 * dq2;
            if (dq1dq2 <= 0)
            {
                return 0.0;
            }
            else
            {
                return 2.0 * dq1dq2 / (dq1 + dq2);
            }
        }

        // Cha & Whitworth (2003) method with optional force correction
        void FluidForce::calculation(std::shared_ptr<Simulation> sim)
        {
            auto &particles = sim->get_particles();
            auto *periodic = sim->get_periodic().get();
            const int num = sim->get_particle_num();
            auto *kernel = sim->get_kernel().get();
            auto *tree = sim->get_tree().get();

            const real dt = sim->get_dt();

            // for MUSCL reconstruction
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

                // neighbor search
#ifdef EXHAUSTIVE_SEARCH
                int const n_neighbor = exhaustive_search(p_i, p_i.sml, particles, num, neighbor_list, m_neighbor_number * neighbor_list_size, periodic, true);
#else
                int const n_neighbor = tree->neighbor_search(p_i, neighbor_list, particles, true);
#endif

                // Fluid force for particle i.
                const vec_t &r_i = p_i.pos;
                const vec_t &v_i = p_i.vel;
                const real h_i = p_i.sml;
                const real rho2_inv_i = 1.0 / sqr(p_i.dens);

                vec_t acc(0.0);
                real dene = 0.0;

                for (int n = 0; n < n_neighbor; ++n)
                {
                    int const j = neighbor_list[n];
                    auto &p_j = particles[j];
                    const vec_t r_ij = periodic->calc_r_ij(r_i, p_j.pos);
                    const real r = std::abs(r_ij);

                    if (r >= std::max(h_i, p_j.sml) || r == 0.0)
                    {
                        continue;
                    }

                    const real r_inv = 1.0 / r;
                    const vec_t e_ij = r_ij * r_inv;
                    const real ve_i = inner_product(v_i, e_ij);
                    const real ve_j = inner_product(p_j.vel, e_ij);

                    // Compute kernel gradients
                    const vec_t dw_i = kernel->dw(r_ij, r, h_i);
                    const vec_t dw_j = kernel->dw(r_ij, r, p_j.sml);
                    const real rho2_inv_j = 1.0 / sqr(p_j.dens);

                    // Shock detection parameters
                    const real du = std::abs(ve_i - ve_j);
                    const real avg_sound = std::max(p_i.sound, p_j.sound);
                    const real shockThreshold = 0.15 * avg_sound;
                    const real divergenceThreshold = 0.0;
                    vec_t dv_i, dv_j;
                    for (int k = 0; k < DIM; ++k)
                    {
                        dv_i[k] = inner_product(grad_v[k][i], e_ij);
                        dv_j[k] = inner_product(grad_v[k][j], e_ij);
                    }
                    const real dve_i = inner_product(dv_i, e_ij);
                    const real dve_j = inner_product(dv_j, e_ij);
                    const real dynamic_threshold = 0.1 * avg_sound;
                    // HLL solver
                    real pstar, vstar;

                    const real right[4] = {ve_i, p_i.dens, p_i.pres, p_i.sound};
                    const real left[4] = {ve_j, p_j.dens, p_j.pres, p_j.sound};
                    m_solver(left, right, pstar, vstar);
                    const vec_t v_ij = v_i - p_j.vel;
                    const vec_t dw_ij = (dw_i + dw_j) * 0.5;

                    real pi_ij = 0;

                    vec_t f_vis = dw_ij * (p_i.mass * (pstar - p_i.pres) * rho2_inv_i) + dw_ij * (p_j.mass * (pstar - p_j.pres) * rho2_inv_j);
                    vec_t f_invis = dw_ij * (p_i.mass * (p_i.pres) * rho2_inv_i) +
                                    dw_ij * (p_j.mass * (p_j.pres) * rho2_inv_j);
                    acc -= f_invis + f_vis * 0.5 * (p_i.balsara + p_j.balsara);

                    vec_t ene_vis = dw_ij * (p_i.mass * (pstar - p_i.pres) * rho2_inv_i); // const vec_t v_ij_gsph = e_ij * vstar;
                    const vec_t v_ij_gsph = e_ij * vstar;
                    vec_t ene_invis = dw_ij * (p_i.mass * (p_i.pres) * rho2_inv_i); // const vec_t v_ij_gsph = e_ij * vstar;
                    dene += inner_product(
                                ene_invis, v_ij) +
                            inner_product(ene_vis, v_ij) * 0.5 * (p_i.balsara + p_j.balsara);
                }
                p_i.acc = acc;
                p_i.dene = dene;
            }
        }

        void FluidForce::hll_solver()
        {
            m_solver = [&](const real left[], const real right[], real &pstar, real &vstar)
            {
                const real u_l = left[0];
                const real rho_l = left[1];
                const real p_l = left[2];
                const real c_l = left[3];

                const real u_r = right[0];
                const real rho_r = right[1];
                const real p_r = right[2];
                const real c_r = right[3];

                const real roe_l = std::sqrt(rho_l);
                const real roe_r = std::sqrt(rho_r);
                const real roe_inv = 1.0 / (roe_l + roe_r);

                const real u_t = (roe_l * u_l + roe_r * u_r) * roe_inv;
                const real c_t = (roe_l * c_l + roe_r * c_r) * roe_inv;
                const real s_l = std::min(u_l - c_l, u_t - c_t);
                const real s_r = std::max(u_r + c_r, u_t + c_t);

                const real c1 = rho_l * (s_l - u_l);
                const real c2 = rho_r * (s_r - u_r);
                const real c3 = 1.0 / (c1 - c2);
                const real c4 = p_l - u_l * c1;
                const real c5 = p_r - u_r * c2;

                vstar = (c5 - c4) * c3;
                pstar = (c1 * c5 - c2 * c4) * c3;
            };
        }

    } // namespace gsph
} // namespace sph
