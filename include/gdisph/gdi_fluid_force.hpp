// File: include/disph/godnouv_disph.hpp
#pragma once

#include "fluid_force.hpp"
#include <functional>

namespace sph
{
    namespace gdisph
    {

        /// GodnouvFluidForce implements GDISPH with a Godunov-type Riemann solver (HLL)
        /// for GDISPH Case 1, without artificial viscosity, consistent with the paper's formulation.
        class GodnouvFluidForce : public sph::FluidForce
        {
        private:
            bool m_is_2nd_order;               // Flag for second-order reconstruction (not used in this version)
            real m_gamma;                      // Adiabatic index
            bool m_use_balsara_switch = false; // Disabled for GDISPH Case 1 (no artificial viscosity)
            // Solver function: takes left and right states (velocity, density, pressure, sound speed)
            // and computes pstar and vstar
            std::function<void(const real[], const real[], real &pstar, real &vstar)> m_solver;

            void hll_solver(); // Sets up the HLL Riemann solver
            void compute_hll_states(const SPHParticle &p_i, const SPHParticle &p_j, const vec_t &r_ij, real left[], real right[]);

        public:
            void initialize(std::shared_ptr<SPHParameters> param) override;
            void calculation(std::shared_ptr<Simulation> sim) override;
        };

    } // namespace gdisph
} // namespace sph