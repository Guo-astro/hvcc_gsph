#ifndef DENSITY_RELAXATION_HPP
#define DENSITY_RELAXATION_HPP
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <cstdlib>
#include <cmath>
#include "core/simulation.hpp" // Contains Simulation class
#include "core/parameters.hpp" // Contains SPHParameters struct

namespace sph
{
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
    // ---------------------------------------------------------------------
    // Helper: compute dθ/dξ at a given ξ using the Lane–Emden lookup table.
    // This function does a simple piecewise-linear interpolation.
    // ---------------------------------------------------------------------
    inline real dTheta_dXi(real xi)
    {
        // Ensure the table is loaded.
        if (laneEmden_x.empty())
            loadLaneEmdenTableFromCSV(); // uses default filename if not loaded

        // If xi is below or above the table range, do a simple slope extrapolation:
        if (xi <= laneEmden_x.front())
        {
            if (laneEmden_x.size() >= 2)
            {
                return (laneEmden_theta[1] - laneEmden_theta[0]) /
                       (laneEmden_x[1] - laneEmden_x[0]);
            }
            else
                return 0.0;
        }
        if (xi >= laneEmden_x.back())
        {
            size_t n = laneEmden_x.size();
            if (n >= 2)
            {
                return (laneEmden_theta[n - 1] - laneEmden_theta[n - 2]) /
                       (laneEmden_x[n - 1] - laneEmden_x[n - 2]);
            }
            else
                return 0.0;
        }

        // Otherwise, find interval [i, i+1] such that laneEmden_x[i] <= xi < laneEmden_x[i+1].
        size_t i = 0;
        for (; i < laneEmden_x.size() - 1; ++i)
        {
            if (xi < laneEmden_x[i + 1])
                break;
        }
        real dx = laneEmden_x[i + 1] - laneEmden_x[i];
        if (dx < 1e-12)
            return 0.0;
        real dtheta = laneEmden_theta[i + 1] - laneEmden_theta[i];
        return dtheta / dx;
    }
    inline real getTheta(real xi)
    {

        // 1) Make sure the table is loaded
        if (laneEmden_x.empty())
        {
            loadLaneEmdenTableFromCSV(); // default or user-specified file
        }
        // 2) If table has fewer than 2 points, just return 0 or the only value we have
        if (laneEmden_x.size() < 2)
        {
            return (laneEmden_x.empty() ? 0.0 : laneEmden_theta.front());
        }

        // 3) Handle xi <= min(x)
        if (xi <= laneEmden_x.front())
        {
            // Extrapolate using the first two points
            real x0 = laneEmden_x[0];
            real x1 = laneEmden_x[1];
            real t0 = laneEmden_theta[0];
            real t1 = laneEmden_theta[1];
            real dx = x1 - x0;
            if (std::fabs(dx) < 1e-12)
            {
                return t0;
            }
            real slope = (t1 - t0) / dx;
            // linear extrapolation
            return t0 + slope * (xi - x0);
        }

        // 4) Handle xi >= max(x)
        if (xi >= laneEmden_x.back())
        {
            // Extrapolate using the last two points
            size_t n = laneEmden_x.size();
            real x0 = laneEmden_x[n - 2];
            real x1 = laneEmden_x[n - 1];
            real t0 = laneEmden_theta[n - 2];
            real t1 = laneEmden_theta[n - 1];
            real dx = x1 - x0;
            if (std::fabs(dx) < 1e-12)
            {
                return t1;
            }
            real slope = (t1 - t0) / dx;
            // linear extrapolation
            return t1 + slope * (xi - x1);
        }

        // 5) Otherwise, xi is within the table. Find [i, i+1] s.t. laneEmden_x[i] <= xi < laneEmden_x[i+1].
        //    A simple linear search is fine if the table is not huge; otherwise, consider binary search.
        size_t i = 0;
        for (; i < laneEmden_x.size() - 1; ++i)
        {
            if (xi < laneEmden_x[i + 1])
                break;
        }

        // 6) Piecewise-linear interpolation in the interval
        real x0 = laneEmden_x[i];
        real x1 = laneEmden_x[i + 1];
        real t0 = laneEmden_theta[i];
        real t1 = laneEmden_theta[i + 1];
        real dx = x1 - x0;
        if (std::fabs(dx) < 1e-12)
        {
            return t0; // or t1, they are effectively the same if x0~x1
        }
        real frac = (xi - x0) / dx;
        return t0 + frac * (t1 - t0);
    }
    void add_relaxation_force(std::shared_ptr<Simulation> sim, const SPHParameters &params);
}

#endif // DENSITY_RELAXATION_HPP