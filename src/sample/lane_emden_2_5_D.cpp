#include <cmath>
#include <vector>
#include <iostream>
#include <fstream>
#include <sstream>
#include <memory>
#include "sample_registry.hpp"
#include "simulation.hpp"
#include "parameters.hpp"
#include "particle.hpp"
#include "exception.hpp"
#include "defines.hpp"

namespace sph
{

    using real = double;

    // Global constant: first zero of theta(xi) for n = 3/2.
    const real xi1_global = 3.65375;

    // Lookup table for the Lane–Emden solution loaded from CSV.
    static std::vector<real> laneEmden_x;
    static std::vector<real> laneEmden_theta;
    static bool laneEmdenTableLoaded = false;

    // Load the Lane–Emden solution from CSV.
    void loadLaneEmdenTableFromCSV(const std::string &filename = "./sample/lane_emden/lane_emden_data_5_3.csv")
    {
        if (laneEmdenTableLoaded)
            return;
        std::ifstream infile(filename);
        if (!infile.is_open())
        {
            std::cerr << "Error: could not open " << filename << " for reading." << std::endl;
            exit(1);
        }
        std::string line;
        // Skip header line.
        std::getline(infile, line);
        while (std::getline(infile, line))
        {
            std::istringstream iss(line);
            std::string token;
            if (std::getline(iss, token, ','))
            {
                real xi = std::stod(token);
                if (std::getline(iss, token, ','))
                {
                    real theta = std::stod(token);
                    laneEmden_x.push_back(xi);
                    laneEmden_theta.push_back(theta);
                }
            }
        }
        laneEmdenTableLoaded = true;
        std::cout << "Loaded Lane–Emden table from " << filename
                  << " with " << laneEmden_x.size() << " entries." << std::endl;
    }

    // Interpolate the Lane–Emden solution from the lookup table.
    real laneEmdenTheta(real xi_target)
    {
        if (!laneEmdenTableLoaded)
            loadLaneEmdenTableFromCSV();
        // For very small xi, use series expansion.
        if (xi_target <= laneEmden_x.front())
            return 1.0 - (xi_target * xi_target) / 6.0;
        if (xi_target >= xi1_global) // Clamp for xi beyond the stellar boundary.
            return 0.0;
        // If xi_target exceeds our table range, return the last value.
        if (xi_target >= laneEmden_x.back())
            return laneEmden_theta.back();
        // Binary search to find the interval.
        int low = 0;
        int high = laneEmden_x.size() - 1;
        while (low <= high)
        {
            int mid = low + (high - low) / 2;
            if (laneEmden_x[mid] < xi_target)
                low = mid + 1;
            else
                high = mid - 1;
        }
        int i = (low - 1 < 0) ? 0 : low - 1;
        real x0 = laneEmden_x[i], x1 = laneEmden_x[i + 1];
        real theta0 = laneEmden_theta[i], theta1 = laneEmden_theta[i + 1];
        real t = (xi_target - x0) / (x1 - x0);
        return theta0 + t * (theta1 - theta0);
    }

    // Modified getTheta using the CSV lookup table.
    real getTheta(real xi_target)
    {
        if (xi_target < 1e-6)
            return 1.0 - (xi_target * xi_target) / 6.0;
        return laneEmdenTheta(xi_target);
    }

    //-------------------------------------------------------------------
    // cumulativeMass: Compute the cumulative mass integral I(xi)
    // I(xi) = ∫₀ˣ [θ(ξ)]^(3/2) ξ² dξ using composite Simpson's rule.
    // This is used for the gravitational (3D) structure.
    //-------------------------------------------------------------------
    real cumulativeMass(real xi_target)
    {
        int n_steps = 1000;
        if (n_steps % 2 != 0)
            n_steps++;
        real h = xi_target / n_steps;
        auto f = [](real xi) -> real
        {
            real theta_val = getTheta(xi);
            return std::pow(theta_val, 1.5) * xi * xi;
        };
        real integral = f(0) + f(xi_target);
        for (int i = 1; i < n_steps; i++)
        {
            real x = i * h;
            integral += (i % 2 == 0) ? 2 * f(x) : 4 * f(x);
        }
        integral *= h / 3.0;
        return integral;
    }

    //-------------------------------------------------------------------
    // invertCumulativeMass: Given u ∈ [0,1], find xi such that
    // F(xi) = I(xi) / I_total = u using bisection.
    //-------------------------------------------------------------------
    real invertCumulativeMass(real u)
    {
        real I_total = cumulativeMass(xi1_global);
        real target = u * I_total;
        real low = 0.0;
        real high = xi1_global;
        real mid = 0.0;
        int iterations = 0;
        const int max_iter = 10;
        const real tol = 1e-6;
        while (iterations < max_iter)
        {
            mid = (low + high) / 2.0;
            real I_mid = cumulativeMass(mid);
            if (std::abs(I_mid - target) < tol)
                break;
            if (I_mid < target)
                low = mid;
            else
                high = mid;
            iterations++;
        }
        return mid;
    }

    void outputSimulationCSV(const std::vector<SPHParticle> &particles, const std::string &filename = "simulation_2_5D_debug.csv")
    {
        std::ofstream out(filename);
        if (!out.is_open())
        {
            std::cerr << "Could not open " << filename << " for writing." << std::endl;
            return;
        }
        // Write header.
        out << "pos_x,pos_y,pos_z,vel_x,vel_y,vel_z,acc_x,acc_y,acc_z,mass,dens,pres,ene,sml,id\n";
        for (const auto &p : particles)
        {
            out << p.pos[0] << "," << p.pos[1] << "," << p.pos[2] << ","
                << p.vel[0] << "," << p.vel[1] << "," << p.vel[2] << ","
                << p.acc[0] << "," << p.acc[1] << "," << p.acc[2] << ","
                << p.mass << "," << p.dens << "," << p.pres << "," << p.ene << ","
                << p.sml << "," << p.id << "\n";
        }
        out.close();
        std::cout << "Simulation data written to " << filename << std::endl;
    }

    //-------------------------------------------------------------------
    // load_lane_emden: Sets up particle properties using the Lane–Emden solution.
    // Modified for a 2.5D hydrodynamic simulation:
    // - Particle positions are initialized on a 2D grid (x–y plane with z=0).
    // - Hydrodynamic interactions (e.g., density and kernel evaluations) are computed in 2D,
    //   while gravitational forces use the full 3D (even though z is zero).
    //-------------------------------------------------------------------
    void load_lane_emden(std::shared_ptr<sph::Simulation> sim,
                         std::shared_ptr<sph::SPHParameters> param)
    {
#if DIM == 3
        // 1) Physical and Lane–Emden parameters.
        const real R_phys = 3.0;     // Cloud radius in pc.
        const real M_total = 1000.0; // Total mass in solar masses.
        const real xi1 = 3.65375;    // First zero of θ(ξ) for n = 3/2.
        const real alpha = R_phys / xi1;
        const real omega_n = 2.714; // Precomputed.
        const real rho_c = M_total / (4.0 * M_PI * std::pow(alpha, 3) * omega_n);
        const real n_poly = 1.5;
        const real gamma = 5.0 / 3.0; // Correct floating-point division.
        const real G = param->gravity.constant;
        const real K = (4.0 * M_PI * G * std::pow(alpha, 2) * std::pow(rho_c, 1.0 / 3.0)) / (n_poly + 1.0);

        // 2) Create Particle Positions.
        // For a 2.5D simulation, we distribute particles on a 2D grid (x-y plane) with z = 0.
        // Gravity is still computed using full 3D positions.
        int N = 20; // Grid resolution in 2D.
        auto &p = sim->get_particles();
        p.clear();
        p.reserve(N * N);
        const real dxi = 2.0 / N;
        std::cout << "Initializing particles in 2.5D (2D grid with z=0)..." << std::endl;
        int totalIterations = N * N;
        int counter = 0;
        for (int i = 0; i < N; i++)
        {
            for (int j = 0; j < N; j++)
            {
                counter++;
                if (counter % 100 == 0 || counter == totalIterations)
                {
                    int progress = static_cast<int>(100.0 * counter / totalIterations);
                    std::cout << "\rProgress: " << progress << "% complete" << std::flush;
                }
                // Create a 2D position in the x-y plane; z is set to 0.
                vec_t r_uniform = {
                    (i + 0.5) * dxi - 1.0,
                    (j + 0.5) * dxi - 1.0,
                    0.0};
                // Use a 2D radial measure (ignoring z).
                real ru_mag = std::sqrt(r_uniform[0] * r_uniform[0] + r_uniform[1] * r_uniform[1]);
                if (ru_mag > 1.0)
                    continue;
                // For 2D, the area element scales as r dr so we take u = r^2.
                real u = ru_mag * ru_mag;
                // Here we use the cumulative mass function from the 3D Lane–Emden profile.
                // Adjust this if you want a different mass distribution in 2D.
                real xi = invertCumulativeMass(u);
                real r_phys = alpha * xi;
                vec_t pos = r_uniform;
                if (ru_mag > 0.0)
                    pos *= (r_phys / ru_mag);
                else
                    pos = {0.0, 0.0, 0.0};
                SPHParticle p_i;
                p_i.pos = pos;
                p.emplace_back(p_i);
            }
        }
        std::cout << "\rProgress: 100% complete" << std::endl;
        std::cout << "Total number of particles: " << p.size() << std::endl;

        // 3) Fill in Particle Properties.
        const size_t total = p.size();
        const real particle_mass = M_total / total;
        for (size_t idx = 0; idx < total; ++idx)
        {
            auto &pp = p[idx];
            // Set initial velocity to zero.
            pp.vel = {0.0, 0.0, 0.0};
            pp.mass = particle_mass;
            // Compute xi from the 3D magnitude (gravity uses full 3D).
            real xi = std::abs(pp.pos) / alpha;
            real theta_val = getTheta(xi);
            pp.dens = rho_c * std::pow(theta_val, n_poly);
            real pres = K * std::pow(pp.dens, 1.0 + 1.0 / n_poly);
            pp.pres = pres;
            pp.ene = pres / ((gamma - 1.0) * pp.dens);
            // In 2D hydrodynamics, it is common to set the smoothing length based on area,
            // e.g., using a square-root scaling.
            pp.sml = std::sqrt(pp.mass / pp.dens);
            pp.id = static_cast<int>(idx);
        }
        sim->set_particle_num(static_cast<int>(p.size()));
        outputSimulationCSV(sim->get_particles());
#endif
    }

    REGISTER_SAMPLE("lane_emden_2_5D", load_lane_emden);

} // end namespace sph
