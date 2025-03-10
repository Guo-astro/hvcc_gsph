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

    // Default 2D boundary fallback (if no CSV is loaded)
    constexpr real xi1_default_2d = 3.351; // Approximate for n=1.5 in 2D

    // Inline variables for 2D Lane-Emden solution
    inline std::vector<real> laneEmden_x_2d;
    inline std::vector<real> laneEmden_theta_2d;
    inline std::string laneEmdenCSVFileLoaded_2d = "";

    // Load the 2D Lane–Emden solution from CSV
    inline void loadLaneEmdenTableFromCSV_2d(const std::string &filename = "./sample/lane_emden_2d/lane_emden_2d.csv")
    {
        if (laneEmdenCSVFileLoaded_2d == filename)
            return;

        laneEmden_x_2d.clear();
        laneEmden_theta_2d.clear();

        std::ifstream infile(filename);
        if (!infile.is_open())
        {
            std::cerr << "Error: could not open " << filename << " for reading." << std::endl;
            std::exit(EXIT_FAILURE);
        }
        std::string line;
        if (!std::getline(infile, line))
        {
            std::cerr << "Error: CSV file " << filename << " is empty." << std::endl;
            std::exit(EXIT_FAILURE);
        }
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
                        laneEmden_x_2d.push_back(xi);
                        laneEmden_theta_2d.push_back(theta);
                    }
                }
            }
            catch (const std::exception &e)
            {
                std::cerr << "Error parsing CSV file " << filename << ": " << e.what() << std::endl;
                std::exit(EXIT_FAILURE);
            }
        }
        laneEmdenCSVFileLoaded_2d = filename;
        std::cout << "Loaded 2D Lane–Emden table from " << filename
                  << " with " << laneEmden_x_2d.size() << " entries." << std::endl;
    }

    // Interpolate the 2D Lane–Emden solution from the lookup table
    inline real laneEmdenTheta_2d(real xi_target)
    {
        if (laneEmden_x_2d.empty())
            loadLaneEmdenTableFromCSV_2d();

        if (xi_target <= laneEmden_x_2d.front())
            return 1.0 - (xi_target * xi_target) / 6.0;

        real xi_boundary = laneEmden_x_2d.back();
        if (xi_target >= xi_boundary)
            return 0.0;

        size_t low = 0;
        size_t high = laneEmden_x_2d.size() - 1;
        while (low <= high)
        {
            size_t mid = low + (high - low) / 2;
            if (laneEmden_x_2d[mid] < xi_target)
                low = mid + 1;
            else
                high = mid - 1;
        }
        size_t i = (low == 0) ? 0 : low - 1;
        real x0 = laneEmden_x_2d[i], x1 = laneEmden_x_2d[i + 1];
        real theta0 = laneEmden_theta_2d[i], theta1 = laneEmden_theta_2d[i + 1];
        real t = (xi_target - x0) / (x1 - x0);
        return theta0 + t * (theta1 - theta0);
    }

    // Get theta value using the 2D CSV lookup table
    inline real getTheta_2d(real xi_target)
    {
        if (xi_target < 1e-6)
            return 1.0 - (xi_target * xi_target) / 6.0;
        return laneEmdenTheta_2d(xi_target);
    }

    // Compute the cumulative mass integral I(xi) for 2D
    inline real cumulativeMass_2d(real xi_target, real n)
    {
        int n_steps = 1000;
        if (n_steps % 2 != 0)
            n_steps++;
        real h = xi_target / n_steps;
        auto f = [n](real xi) -> real
        {
            real theta_val = getTheta_2d(xi);
            return std::pow(theta_val, n) * xi; // 2D: θ^n * ξ
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

    // Invert the cumulative mass function for 2D
    inline real invertCumulativeMass_2d(real u, real n)
    {
        real xi_boundary = laneEmden_x_2d.empty() ? xi1_default_2d : laneEmden_x_2d.back();
        real I_total = cumulativeMass_2d(xi_boundary, n);
        real target = u * I_total;
        real low = 0.0;
        real high = xi_boundary;
        real mid = 0.0;
        int iterations = 0;
        const int max_iter = 100;
        const real tol = 1e-6;
        while (iterations < max_iter)
        {
            mid = (low + high) / 2.0;
            real I_mid = cumulativeMass_2d(mid, n);
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

    // Output particle simulation data to CSV for 2D simulations
    inline void outputSimulationCSV_2d(const std::vector<SPHParticle> &particles, const std::string &filename = "simulation_debug_2d.csv")
    {
        std::ofstream out(filename);
        if (!out.is_open())
        {
            std::cerr << "Could not open " << filename << " for writing." << std::endl;
            return;
        }
        out << "pos_x,pos_y,vel_x,vel_y,acc_x,acc_y,mass,dens,pres,ene,sml,id\n";
        for (const auto &p : particles)
        {
            out << p.pos[0] << "," << p.pos[1] << ","
                << p.vel[0] << "," << p.vel[1] << ","
                << p.acc[0] << "," << p.acc[1] << ","
                << p.mass << "," << p.dens << "," << p.pres << "," << p.ene << ","
                << p.sml << "," << p.id << "\n";
        }
        out.close();
        std::cout << "Simulation data written to " << filename << std::endl;
    }

} // namespace sph