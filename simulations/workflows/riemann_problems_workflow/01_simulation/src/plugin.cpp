/**
 * @file plugin.cpp
 * @brief 1D Riemann Problems Test Suite
 * 
 * Implements multiple 1D Riemann problems for SPH benchmarking:
 * - Test 1: Sod shock tube (standard)
 * - Test 2: Double rarefaction (123 problem)
 * - Test 3: Strong shock (Mach 100)
 * - Test 4: Slow shock (mild discontinuity)
 * - Test 5: Vacuum generation
 * 
 * Reference: Toro (2009) "Riemann Solvers and Numerical Methods for Fluid Dynamics"
 */

#include "core/simulation_plugin.hpp"
#include "core/simulation.hpp"
#include "core/parameters.hpp"
#include "core/particle.hpp"
#include "utilities/exception.hpp"
#include "utilities/defines.hpp"
#include <iostream>
#include <cmath>
#include <vector>
#include <cstdlib>

namespace sph {

class RiemannProblemsPlugin : public SimulationPlugin {
public:
    std::string get_name() const override {
        return "riemann_problems";
    }
    
    std::string get_description() const override {
        return "1D Riemann problems benchmark suite (Tests 1-5)";
    }
    
    std::string get_version() const override {
        return "1.0.0";
    }
    
    void initialize(std::shared_ptr<Simulation> sim,
                   std::shared_ptr<SPHParameters> param) override {
#if DIM != 1
        THROW_ERROR("Riemann problems require DIM=1");
#endif

        std::cout << "\n=== 1D Riemann Problems Test Suite ===" << std::endl;
        
        // Get test case from environment variable (default: 1)
        int test_case = 1;
        if (const char* env_test = std::getenv("TEST_CASE")) {
            test_case = std::atoi(env_test);
        }
        
        std::cout << "Test Case: " << test_case << std::endl;
        
        // Particle resolution
        int N = 400;  // Total number of particles
        
        // Domain: [-0.5, 0.5], discontinuity at x=0
        const real x_min = -0.5;
        const real x_max = 0.5;
        const real domain_length = x_max - x_min;
        const real dx = domain_length / N;
        
        // Physical parameters
        const real gamma = param->physics.gamma;
        const real x_interface = 0.0;
        
        // Initial conditions based on test case
        real rho_L, P_L, v_L;
        real rho_R, P_R, v_R;
        std::string test_name;
        
        switch (test_case) {
            case 1:  // Sod shock tube (standard)
                test_name = "Sod Shock Tube";
                rho_L = 1.0;   P_L = 1.0;   v_L = 0.0;
                rho_R = 0.125; P_R = 0.1;   v_R = 0.0;
                break;
                
            case 2:  // Double rarefaction (123 problem)
                test_name = "Double Rarefaction (123 Problem)";
                rho_L = 1.0;   P_L = 0.4;   v_L = -2.0;
                rho_R = 1.0;   P_R = 0.4;   v_R = 2.0;
                break;
                
            case 3:  // Strong shock
                test_name = "Strong Shock (Mach 100)";
                rho_L = 1.0;   P_L = 1000.0; v_L = 0.0;
                rho_R = 1.0;   P_R = 0.01;   v_R = 0.0;
                break;
                
            case 4:  // Slow shock
                test_name = "Slow Shock";
                rho_L = 1.0;   P_L = 0.4;   v_L = -0.5;
                rho_R = 1.0;   P_R = 0.4;   v_R = 0.5;
                break;
                
            case 5:  // Vacuum generation
                test_name = "Vacuum Generation";
                rho_L = 1.0;   P_L = 1.0;   v_L = -1.0;
                rho_R = 1.0;   P_R = 1.0;   v_R = 1.0;
                break;
                
            default:
                std::cerr << "Unknown test case: " << test_case << std::endl;
                std::cerr << "Using default: Sod shock tube" << std::endl;
                test_name = "Sod Shock Tube (Default)";
                rho_L = 1.0;   P_L = 1.0;   v_L = 0.0;
                rho_R = 0.125; P_R = 0.1;   v_R = 0.0;
        }
        
        std::cout << "Test: " << test_name << std::endl;
        std::cout << "Left state:  ρ=" << rho_L << ", P=" << P_L << ", v=" << v_L << std::endl;
        std::cout << "Right state: ρ=" << rho_R << ", P=" << P_R << ", v=" << v_R << std::endl;
        
        // Create particles
        std::vector<SPHParticle> p(N);
        
        for (int i = 0; i < N; ++i) {
            auto &p_i = p[i];
            
            // Position: uniform spacing
            p_i.pos[0] = x_min + (i + 0.5) * dx;
            
            // Set state based on position (discontinuity at x_interface)
            if (p_i.pos[0] < x_interface) {
                // Left state
                p_i.dens = rho_L;
                p_i.pres = P_L;
                p_i.vel[0] = v_L;
                p_i.mass = rho_L * dx;  // mass = ρ * dx in 1D
            } else {
                // Right state
                p_i.dens = rho_R;
                p_i.pres = P_R;
                p_i.vel[0] = v_R;
                p_i.mass = rho_R * dx;  // mass = ρ * dx in 1D
            }
            
            // Specific internal energy
            p_i.ene = p_i.pres / ((gamma - 1.0) * p_i.dens);
            p_i.id = i;
            
            // Initialize volume element for DISPH: V = m/ρ
            p_i.volume = p_i.mass / p_i.dens;
        }
        
        sim->set_particles(p);
        sim->set_particle_num(N);
        
        std::cout << "Created " << N << " particles" << std::endl;
        std::cout << "  Domain: [" << x_min << ", " << x_max << "]" << std::endl;
        std::cout << "  Particle spacing: " << dx << std::endl;
        std::cout << "  SPH Type: " << (param->type == SPHType::GDISPH ? "GDISPH" : 
                                       param->type == SPHType::SSPH ? "SSPH" : 
                                       param->type == SPHType::DISPH ? "DISPH" : "UNKNOWN") << std::endl;
        std::cout << "=== Initialization Complete ===" << std::endl;
    }
    
    std::vector<std::string> get_source_files() const override {
        return {"riemann_problems.cpp"};
    }
};

} // namespace sph

// Export the plugin
DEFINE_SIMULATION_PLUGIN(sph::RiemannProblemsPlugin)
