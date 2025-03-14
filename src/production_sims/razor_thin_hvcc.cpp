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
    class InfallModifier : public CheckpointModifier
    {
    public:
        vec_t target;     // Target position (e.g., static IMBH location)
        real infallSpeed; // Desired infall speed (from param->initial_velocity)
        real pointMass;   // Mass of the BH particle.
        bool bhAdded;     // Flag to ensure the BH particle is added only once.

        InfallModifier(const vec_t &target_, real speed, real pointMass_)
            : target(target_), infallSpeed(speed), pointMass(pointMass_), bhAdded(false)
        {
        }

        virtual void modifyParticles(std::vector<SPHParticle> &particles,
                                     std::shared_ptr<Simulation> sim) override
        {
            // Add the static BH particle if it hasn't been added yet.
            if (!bhAdded)
            {
                SPHParticle bh;
                // Set the BH's position at the target.
                bh.pos[0] = target[0];
                bh.pos[1] = target[1];
                bh.pos[2] = target[2];

                // Set static (zero) velocity.
                bh.vel[0] = 0.0;
                bh.vel[1] = 0.0;
                bh.vel[2] = 0.0;

                // Set the mass.
                bh.mass = pointMass;

                // For a collisionless point mass, hydrodynamic properties are irrelevant.
                bh.dens = 0.0;
                bh.pres = 0.0;
                bh.ene = 0.0;
                bh.sml = 1e-6;

                // Mark as special so that fluid modules ignore it.
                bh.is_point_mass = true;
                bh.id = sim->get_particles().size();

                particles.push_back(bh);
                sim->set_particle_num(particles.size());
                bhAdded = true;

                std::cout << "Added static IMBH particle with mass " << bh.mass
                          << " solar masses at position (" << bh.pos[0] << ", "
                          << bh.pos[1] << ", " << bh.pos[2] << ").\n";
            }

            // Modify fluid particles to make them "fall" toward the target.
            for (auto &p : particles)
            {
                // Skip modifying the BH (point mass) itself.
                if (p.is_point_mass)
                    continue;

                // Compute direction from the particle to the target.
                // vec_t direction;
                // direction[0] = target[0] - p.pos[0];
                // direction[1] = target[1] - p.pos[1];
                // direction[2] = target[2] - p.pos[2];

                // // Normalize the direction.
                // real norm = std::sqrt(direction[0] * direction[0] +
                //                       direction[1] * direction[1] +
                //                       direction[2] * direction[2]);
                p.vel[1] += infallSpeed;
                p.pos[0] += 60.0;
            }
        }
    };

    /// \brief Sample: load_razor_thin_hvcc
    ///
    /// This sample sets the checkpoint file for loading the HVCC (razor–thin disk)
    /// data and installs an InfallModifier that forces the disk particles to fall
    /// toward the target at (0, impact_parameter, 0) with speed given by initial_velocity.
    /// The modifier now also adds a static IMBH particle (with mass point_mass) at that target.
    void load_razor_thin_hvcc(std::shared_ptr<Simulation> sim,
                              std::shared_ptr<SPHParameters> param)
    {
        // --- Set flyby/HVCC parameters if not provided.
        if (param->impact_parameter <= 0.0)
        {
            // Default impact parameter in parsecs.
            param->impact_parameter = 1.0;
        }
        if (param->initial_velocity <= 0.0)
        {
            // Default infall speed for the disk in km/s.
            param->initial_velocity = 10.0;
        }
        if (param->point_mass <= 0.0)
        {
            // Default mass for the IMBH in solar masses.
            param->point_mass = 1e5;
        }

        std::cout << "Flyby parameters:\n"
                  << "  Impact parameter: " << param->impact_parameter << " pc\n"
                  << "  Infall speed: " << param->initial_velocity << " km/s\n"
                  << "  Point mass: " << param->point_mass << " solar masses\n";

        // --- Set the InfallModifier.
        // We assume the static IMBH will be located at (0, impact_parameter, 0).
        vec_t target;
        target[0] = 0.0;
        target[1] = 20.0; // Alternatively, you might use param->impact_parameter.
        target[2] = 0.0;
        std::shared_ptr<CheckpointModifier> mod =
            std::make_shared<InfallModifier>(target, param->initial_velocity, param->point_mass);
        sim->set_checkpoint_modifier(mod);

        // --- Note on Checkpoint Loading ---
        // When the solver initializes, if param->checkpoint_file is nonempty,
        // Output::read_checkpoint(param->checkpoint_file, sim) is called.
        // That function will load the disk particle data from the CSV file and then
        // invoke our InfallModifier to adjust the fluid velocities and add the BH particle.
    }

    REGISTER_SAMPLE("razor_thin_hvcc", load_razor_thin_hvcc);
} // namespace sph
