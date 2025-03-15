#include <cmath>
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
    // InfallModifier inherits from your CheckpointModifier interface.
    // It sets the velocity of all fluid particles so that they fall toward the target,
    // and it adds a static BH particle (IMBH) if it has not already been added.
    class DebugModifier : public CheckpointModifier
    {
    public:
        DebugModifier()
        {
        }

        virtual void modifyParticles(std::vector<SPHParticle> &particles,
                                     std::shared_ptr<Simulation> sim) override
        {
        }
    };

    /// \brief Sample: load_razor_thin_hvcc
    ///
    /// This sample sets the checkpoint file for loading the HVCC (razor–thin disk)
    /// data and installs an InfallModifier that forces the disk particles to fall
    /// toward the target at (0, impact_parameter, 0) with speed given by initial_velocity.
    /// The modifier now also adds a static IMBH particle (with mass point_mass) at that target.
    void razor_thin_hvcc_debug(std::shared_ptr<Simulation> sim,
                               std::shared_ptr<SPHParameters> param)
    {

        std::shared_ptr<CheckpointModifier> mod =
            std::make_shared<DebugModifier>();
        sim->set_checkpoint_modifier(mod);

        // --- Note on Checkpoint Loading ---
        // When the solver initializes, if param->checkpoint_file is nonempty,
        // Output::read_checkpoint(param->checkpoint_file, sim) is called.
        // That function will load the disk particle data from the CSV file and then
        // invoke our InfallModifier to adjust the fluid velocities and add the BH particle.
    }

    REGISTER_SAMPLE("razor_thin_hvcc_debug", razor_thin_hvcc_debug);
} // namespace sph
