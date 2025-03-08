#pragma once
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <cstdlib>
#include <cmath>
#include "particle.hpp"

namespace sph
{
    using real = double;

    // Default 3D boundary fallback (if no CSV is loaded)
    constexpr real xi1_default_3d = 3.65375;

    // Inline variables (C++17) so that there is only one instance across translation units.
    inline std::vector<real> laneEmden_x;
    inline std::vector<real> laneEmden_theta;
    inline std::string laneEmdenCSVFileLoaded = "";

    // Load the Lane–Emden solution from CSV.
    inline void loadLaneEmdenTableFromCSV(const std::string &filename = "")
    {
        // If already loaded from this file, do nothing.
        if (laneEmdenCSVFileLoaded == filename)
            return;

        // Clear any previous data.
        laneEmden_x.clear();
        laneEmden_theta.clear();

        std::ifstream infile(filename);
        if (!infile.is_open())
        {
            std::cerr << "Error: could not open " << filename << " for reading." << std::endl;
            std::exit(EXIT_FAILURE);
        }
        std::string line;
        // Skip header line.
        if (!std::getline(infile, line))
        {
            std::cerr << "Error: CSV file " << filename << " is empty." << std::endl;
            std::exit(EXIT_FAILURE);
        }
        // Read CSV rows.
        while (std::getline(infile, line))
        {
            std::istringstream iss(line);
            std::string token;
            try
            {
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
            catch (const std::exception &e)
            {
                std::cerr << "Error parsing CSV file " << filename << ": " << e.what() << std::endl;
                std::exit(EXIT_FAILURE);
            }
        }
        laneEmdenCSVFileLoaded = filename;
        std::cout << "Loaded Lane–Emden table from " << filename
                  << " with " << laneEmden_x.size() << " entries." << std::endl;
    }

    // Interpolate the Lane–Emden solution from the lookup table.
    inline real laneEmdenTheta(real xi_target)
    {
        if (laneEmden_x.empty())
            loadLaneEmdenTableFromCSV();

        if (xi_target <= laneEmden_x.front())
            return 1.0 - (xi_target * xi_target) / 6.0;

        real xi_boundary = laneEmden_x.back();
        if (xi_target >= xi_boundary)
            return 0.0;
        if (xi_target >= laneEmden_x.back())
            return laneEmden_theta.back();

        size_t low = 0;
        size_t high = laneEmden_x.size() - 1;
        while (low <= high)
        {
            size_t mid = low + (high - low) / 2;
            if (laneEmden_x[mid] < xi_target)
                low = mid + 1;
            else
                high = mid - 1;
        }
        size_t i = (low == 0) ? 0 : low - 1;
        real x0 = laneEmden_x[i], x1 = laneEmden_x[i + 1];
        real theta0 = laneEmden_theta[i], theta1 = laneEmden_theta[i + 1];
        real t = (xi_target - x0) / (x1 - x0);
        return theta0 + t * (theta1 - theta0);
    }

    // Get theta value using the CSV lookup table.
    inline real getTheta(real xi_target)
    {
        if (xi_target < 1e-6)
            return 1.0 - (xi_target * xi_target) / 6.0;
        return laneEmdenTheta(xi_target);
    }

    //-------------------------------------------------------------------
    // cumulativeMass: Compute the cumulative mass integral I(xi)
    // I(xi) = ∫₀ˣ [θ(ξ)]^(3/2) ξ² dξ using composite Simpson's rule.
    //-------------------------------------------------------------------
    inline real cumulativeMass(real xi_target)
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
    inline real invertCumulativeMass(real u)
    {
        real xi_boundary = laneEmden_x.empty() ? xi1_default_3d : laneEmden_x.back();
        real I_total = cumulativeMass(xi_boundary);
        real target = u * I_total;
        real low = 0.0;
        real high = xi_boundary;
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

    // Output particle simulation data to CSV.
    inline void outputSimulationCSV(const std::vector<SPHParticle> &particles, const std::string &filename = "simulation_debug.csv")
    {
        std::ofstream out(filename);
        if (!out.is_open())
        {
            std::cerr << "Could not open " << filename << " for writing." << std::endl;
            return;
        }
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
} // namespace sph
