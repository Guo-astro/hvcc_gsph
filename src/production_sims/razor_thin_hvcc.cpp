#pragma once
#include <vector>
#include <iostream>

#include "sample_registry.hpp"
#include "simulation.hpp"
#include "parameters.hpp"
#include "particle.hpp"
#include "defines.hpp"
#include "exception.hpp"
#include "utilities/checkpoint_modifier.hpp"

namespace sph
{
    // NoPressureModifier does not change particle pressures.
    class NoPressureModifier : public CheckpointModifier
    {
    public:
        // Simply leave modifyParticles empty so that all particles remain unchanged.
        virtual void modifyParticles(std::vector<SPHParticle> &particles,
                                     std::shared_ptr<Simulation> sim) override
        {
            // No modification: leave pressure as is.
        }
    };

    // Sample function that reads an existing checkpoint (razor-thin disk data)
    // without introducing any pressure differences.
    void razor_thin_no_pressure_diff(std::shared_ptr<Simulation> sim,
                                     std::shared_ptr<SPHParameters> param)
    {
#if DIM != 3
        THROW_ERROR("razor_thin_no_pressure_diff requires DIM == 3.");
#endif

        if (param->checkpoint_file.empty())
        {
            THROW_ERROR("No checkpoint file provided for initialization.");
        }
        std::cout << "[razor_thin_no_pressure_diff] Reading checkpoint from "
                  << param->checkpoint_file << "\n";

        // Use a no-op modifier so the particle pressures remain unaltered.
        std::shared_ptr<CheckpointModifier> mod = std::make_shared<NoPressureModifier>();
        sim->set_checkpoint_modifier(mod);

        // (The checkpoint reading is assumed to be triggered externally.)
        std::cout << "[razor_thin_no_pressure_diff] Finished reading checkpoint data.\n";
    }

    REGISTER_SAMPLE("razor_thin_no_pressure_diff", razor_thin_no_pressure_diff);
}
