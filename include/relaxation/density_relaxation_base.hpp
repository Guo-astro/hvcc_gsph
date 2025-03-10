#pragma once
#include <memory>
#include "simulation.hpp"
#include "parameters.hpp"

namespace sph
{
    /// Abstract base class for density relaxation.
    /// Child classes must implement add_relaxation_force().
    class DensityRelaxationBase
    {
    public:
        virtual ~DensityRelaxationBase() = default;

        /// Called to apply the “density relaxation” artificial force to the simulation particles.
        virtual void add_relaxation_force(std::shared_ptr<Simulation> sim,
                                          const SPHParameters &params) = 0;
    };
}
