#pragma once

#include "utilities/vector_type.hpp"

namespace sph
{

    class SPHParticle
    {
    public:
        vec_t pos;       // position
        vec_t vel;       // velocity
        vec_t vel_p;     // velocity at t + dt/2
        vec_t acc;       // acceleration
        real mass;       // mass
        real dens;       // mass density
        real pres;       // pressure
        real ene;        // internal energy
        int ene_floored; // Flag: 1 if energy floored, 0 otherwise
        real ene_p;      // internal energy at t + dt/2
        real dene;       // du/dt
        real sml;        // smoothing length
        real sound;      // sound speed

        real balsara; // balsara switch
        real alpha;   // AV coefficient

        real gradh; // grad-h term
        real volume; // volume element V = m/ρ (for DISPH/GDISPH)
        real q; // smoothed internal energy density (for DISPH/GDISPH): q = Σ_j (m_j*u_j)*W_ij

        real phi = 0.0;             // potential
        bool is_point_mass = false; // Flag to indicate if particle is fixed

        int id;
        int neighbor;
        SPHParticle *next = nullptr;
        bool is_wall = false; // <<-- flag indicating a wall particle

        real shockSensor; // dimensionless measure of compression
        int shockMode;    // 1 = currently in shock mode, 0 = not
        int oldShockMode;
        bool switch_to_no_shock_region = false; // True if DISPH will be used
        real target_rho;
        
        // Constructor with default initialization to prevent uninitialized members
        SPHParticle() 
            : pos{}, vel{}, vel_p{}, acc{},
              mass(0.0), dens(0.0), pres(0.0), ene(0.0),
              ene_floored(0), ene_p(0.0), dene(0.0),
              sml(0.1),  // CRITICAL: Default smoothing length (prevents NaN/inf)
              sound(0.0), balsara(0.0), alpha(2.0),
              gradh(1.0), volume(0.0), q(0.0),
              phi(0.0), is_point_mass(false),
              id(0), neighbor(0), next(nullptr), is_wall(false),
              shockSensor(0.0), shockMode(0), oldShockMode(0),
              switch_to_no_shock_region(false), target_rho(0.0)
        {}
    };;

}