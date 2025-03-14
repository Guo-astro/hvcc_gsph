#pragma once
#include <cmath>
#include <vector>
#include <iostream>
#include <algorithm>

#include "sample_registry.hpp"
#include "simulation.hpp"
#include "parameters.hpp"
#include "particle.hpp"
#include "defines.hpp"
#include "exception.hpp"
#include "utilities/checkpoint_modifier.hpp"

namespace sph
{
    // NullModifier sets the pressure to a high value for all particles
    // whose (x,y) position lies within a circle of a given radius.
    class NullModifier : public CheckpointModifier
    {
    public:
        // Center (in x and y) of the circular high-pressure region.
        real center[DIM];
        // Radius of the high-pressure region.
        real radius;
        // The pressure value to set inside the region.
        real highPressure;

        NullModifier(const real center_[DIM], real radius_, real highPressure_)
            : radius(radius_), highPressure(highPressure_)
        {
            WRITE_LOG << "[RazorThinSGModifier] Initializing";

            for (int i = 0; i < DIM; ++i)
                center[i] = center_[i];
        }

        virtual void modifyParticles(std::vector<SPHParticle> &particles,
                                     std::shared_ptr<Simulation> sim) override
        {
            WRITE_LOG << "[RazorThinSGModifier] Initializing";
        }
    };

    void razor_thin_sg_relaxation(std::shared_ptr<Simulation> sim,
                                  std::shared_ptr<SPHParameters> param)
    {
        WRITE_LOG << "[razor_thin_sg_relaxation] Reading checkpoint from " << param->checkpoint_file;

#if DIM != 3
        THROW_ERROR("blast_wave_from_checkpoint requires DIM == 3.");
#endif

        if (param->checkpoint_file.empty())
        {
            THROW_ERROR("No checkpoint file provided for blast wave initialization.");
        }
        WRITE_LOG << "[razor_thin_sg_relaxation] Checkpoint file: " << param->checkpoint_file;

        // Set up a high-pressure modifier.
        real center_arr[DIM] = {0.0};
#if DIM >= 2
        center_arr[1] = 0.0;
#endif
        std::shared_ptr<CheckpointModifier> mod = std::make_shared<NullModifier>(center_arr, 0.2, 1000.0);

        // Debug print: print the simulation pointer and modifier pointer when setting.
        WRITE_LOG << "[razor_thin_sg_relaxation] Setting checkpoint modifier for simulation at " << sim.get()
                  << ", modifier pointer: " << mod.get();
        sim->set_checkpoint_modifier(mod);
        WRITE_LOG << "[razor_thin_sg_relaxation] Checkpoint modifier set successfully.";

        std::cout << "[razor_thin_sg_relaxation] Finished modifying checkpoint data.\n";
    }

    REGISTER_SAMPLE("razor_thin_sg_relaxation", razor_thin_sg_relaxation);
}
