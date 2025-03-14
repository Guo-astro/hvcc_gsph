﻿#include <iostream>

#include "kernel/cubic_spline.hpp"
#include "kernel/wendland_kernel.hpp"

// 数値微分と比較する
#include <iostream>
#include "sample_registry.hpp"
#include "simulation.hpp"
#include "parameters.hpp"

// Define a function that will perform your kernel tests.
namespace sph
{

    // This function will be called when you run the "kernel_test" sample.
    void kernel_test(std::shared_ptr<Simulation> sim, std::shared_ptr<SPHParameters> param)
    {
        std::cout << "Running kernel test..." << std::endl;

        std::ios_base::sync_with_stdio(false);

        constexpr int n_max = 10000;

        // spline
        {
            std::cout << "cubic spline" << std::endl;
            sph::Spline::Cubic cs;

            std::cout << "compare dw(x)/dx to (w(x + dx/2) - w(x - dx/2)) / dx" << std::endl;
            for (int n = 100; n < n_max; n *= 2)
            {
                const real dx = 1.0 / n;
                real error = 0.0;
                for (real x = dx * 0.5; x < 1.0; x += dx)
                {
                    const vec_t r(x);
                    auto dw1 = cs.dw(r, x, 1.0);
                    auto dw2 = (cs.w(x + dx * 0.5, 1.0) - cs.w(x - dx * 0.5, 1.0)) / dx;
                    error += std::abs(dw1[0] - dw2);
                }
                error /= n;
                std::cout << "error (n = " << n << "): " << error << std::endl;
            }

            std::cout << "compare dw(h)/dh to (w(h + dh/2) - w(h - dh/2)) / dh" << std::endl;
            for (int n = 100; n < n_max; n *= 2)
            {
                const real dx = 1.0 / n;
                real error = 0.0;
                for (real x = dx * 0.5; x < 1.0; x += dx)
                {
                    const vec_t r(x);
                    auto dw1 = cs.dhw(x, 1.0);
                    auto dw2 = (cs.w(x, 1.0 + dx * 0.5) - cs.w(x, 1.0 - dx * 0.5)) / dx;
                    error += std::abs(dw1 - dw2);
                }
                error /= n;
                std::cout << "error (n = " << n << "): " << error << std::endl;
            }
        }

        std::cout << std::endl;

        // wendland
        {
            std::cout << "Wendland C4" << std::endl;
            sph::Wendland::C4Kernel wl;

            std::cout << "compare dw(x)/dx to (w(x + dx/2) - w(x - dx/2)) / dx" << std::endl;
            for (int n = 100; n < n_max; n *= 2)
            {
                const real dx = 1.0 / n;
                real error = 0.0;
                for (real x = dx * 0.5; x < 1.0; x += dx)
                {
                    const vec_t r(x);
                    auto dw1 = wl.dw(r, x, 1.0);
                    auto dw2 = (wl.w(x + dx * 0.5, 1.0) - wl.w(x - dx * 0.5, 1.0)) / dx;
                    error += std::abs(dw1[0] - dw2);
                }
                error /= n;
                std::cout << "error (n = " << n << "): " << error << std::endl;
            }

            std::cout << "compare dw(h)/dh to (w(h + dh/2) - w(h - dh/2)) / dh" << std::endl;
            for (int n = 100; n < n_max; n *= 2)
            {
                const real dx = 1.0 / n;
                real error = 0.0;
                for (real x = dx * 0.5; x < 1.0; x += dx)
                {
                    const vec_t r(x);
                    auto dw1 = wl.dhw(x, 1.0);
                    auto dw2 = (wl.w(x, 1.0 + dx * 0.5) - wl.w(x, 1.0 - dx * 0.5)) / dx;
                    error += std::abs(dw1 - dw2);
                }
                error /= n;
                std::cout << "error (n = " << n << "): " << error << std::endl;
            }
        }
    }

    // Register the sample with the sample registry.
    REGISTER_SAMPLE("kernel_test", kernel_test);

} // namespace sph
