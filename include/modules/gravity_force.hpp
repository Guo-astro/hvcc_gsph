#pragma once

#include "modules/module.hpp"
#include "utilities/vector_type.hpp"

namespace sph
{
    class SPHParticle;

    class GravityForce : public Module
    {
        bool m_is_valid;
        real m_constant;

    public:
        void initialize(std::shared_ptr<SPHParameters> param) override;
        void calculation(std::shared_ptr<Simulation> sim) override;
        void apply_self_gravity(std::shared_ptr<Simulation> sim);     // Using BHTree
        void apply_external_gravity(std::shared_ptr<Simulation> sim); // Direct summation for point masses
    };
}
