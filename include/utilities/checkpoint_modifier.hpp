#pragma once
#include <memory>
#include "particle.hpp"

namespace sph
{
    // Forward declaration of Simulation to break the cycle.
    class Simulation;
    /// Abstract interface for checkpoint modification.
    /// A sample‐specific modifier will implement modifyParticles().
    class CheckpointModifier
    {
    public:
        virtual ~CheckpointModifier() = default;

        /// Modify the loaded particles. This function is called immediately
        /// after a checkpoint CSV is read.
        virtual void modifyParticles(std::vector<SPHParticle> &particles,
                                     std::shared_ptr<Simulation> sim) = 0;
    };
}
