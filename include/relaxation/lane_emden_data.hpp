#pragma once

#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <cmath>
#include <algorithm>
#include "defines.hpp"

namespace sph
{
    /// This struct holds data and utility for Lane–Emden solutions (n=1.5).
    /// It merges what used to be in lane_emden.hpp, lane_emden_2d.hpp.
    struct LaneEmdenData
    {
        // Single table storing "xi" and "theta" columns.
        // For 2D vs. 3D, you can keep separate tables or unify;
        // here we unify for demonstration.
        std::vector<real> xi_table;
        std::vector<real> theta_table;

        bool loaded = false;
        std::string loaded_file;

        // Load Lane–Emden CSV from disk.
        // Format assumed: line 1 is a header, subsequent lines have "xi,theta".
        void load_csv(const std::string &filename)
        {
            if (loaded && filename == loaded_file)
                return; // already loaded
            xi_table.clear();
            theta_table.clear();

            std::ifstream infile(filename);
            if (!infile.is_open())
            {
                std::cerr << "Error: Could not open " << filename << "\n";
                std::exit(EXIT_FAILURE);
            }
            std::string line;
            // skip header
            if (!std::getline(infile, line))
            {
                std::cerr << "Error: LaneEmdenData CSV " << filename << " is empty\n";
                std::exit(EXIT_FAILURE);
            }
            while (std::getline(infile, line))
            {
                std::istringstream iss(line);
                std::string token;
                if (std::getline(iss, token, ','))
                {
                    real xVal = std::stod(token);
                    if (std::getline(iss, token, ','))
                    {
                        real tVal = std::stod(token);
                        xi_table.push_back(xVal);
                        theta_table.push_back(tVal);
                    }
                }
            }
            infile.close();
            loaded = true;
            loaded_file = filename;
            std::cout << "LaneEmdenData: loaded " << xi_table.size() << " entries from "
                      << filename << std::endl;
        }

        // Interpolate θ(ξ)
        // If out of bounds, do a simple linear extrapolation.
        real get_theta(real xi) const
        {
            if (!loaded || xi_table.size() < 2)
            {
                // fallback
                return (xi < 1.0e-6) ? 1.0 : 0.0;
            }
            if (xi <= xi_table.front())
            {
                // extrapolate with first segment
                real x0 = xi_table[0];
                real x1 = xi_table[1];
                real t0 = theta_table[0];
                real t1 = theta_table[1];
                real slope = (t1 - t0) / (x1 - x0);
                return t0 + slope * (xi - x0);
            }
            if (xi >= xi_table.back())
            {
                // extrapolate with last segment
                size_t n = xi_table.size();
                real x0 = xi_table[n - 2];
                real x1 = xi_table[n - 1];
                real t0 = theta_table[n - 2];
                real t1 = theta_table[n - 1];
                real slope = (t1 - t0) / (x1 - x0);
                return t1 + slope * (xi - x1);
            }
            // else find bracket
            // simple linear search or binary:
            size_t i = 0;
            for (; i < xi_table.size() - 1; ++i)
            {
                if (xi < xi_table[i + 1])
                    break;
            }
            real x0 = xi_table[i];
            real x1 = xi_table[i + 1];
            real t0 = theta_table[i];
            real t1 = theta_table[i + 1];
            real frac = (xi - x0) / (x1 - x0);
            return t0 + frac * (t1 - t0);
        }

        // Derivative dθ/dξ if needed
        real dtheta_dxi(real xi) const
        {
            // do a small numeric difference or piecewise derivative
            const real eps = 1e-5;
            return (get_theta(xi + eps) - get_theta(xi - eps)) / (2 * eps);
        }
    };
}
