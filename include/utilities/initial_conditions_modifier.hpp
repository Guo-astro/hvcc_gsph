#pragma once
#include <memory>
#include "core/particle.hpp"

namespace sph
{
    // Forward declaration of Simulation to break the cycle.
    class Simulation;
    
    /// Abstract interface for initial conditions modification.
    /// A sample-specific modifier will implement modifyParticles().
    class InitialConditionsModifier
    {
    public:
        virtual ~InitialConditionsModifier() = default;

        /// Modify the loaded particles. This function is called immediately
        /// after initial conditions (CSV/checkpoint file) are read.
        virtual void modifyParticles(std::vector<SPHParticle> &particles,
                                     std::shared_ptr<Simulation> sim) = 0;
    };
}
