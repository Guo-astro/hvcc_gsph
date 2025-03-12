#pragma once

#include "kernel_function.hpp"
#include "vector_type.hpp"
#include "defines.hpp"
#include <cmath>
#include <cassert>

namespace sph
{
    namespace Spline
    {

        // Cubic Spline Kernel (Monaghan & Lattanzio, 1985)
        // Typically defined for 1D, 2D, or 3D.
        class Cubic : public KernelFunction
        {
        private:
            bool m_is2p5; // Optional flag for 2.5D mode; you can use it or ignore it in your formulas.
        public:
            // Constructor that accepts a bool argument.
            explicit Cubic(bool is2p5 = false) : m_is2p5(is2p5) {}

            // Implementation of the interface methods.
            virtual real w(const real r, const real h) const override
            {
                const real q = r / h;
                real sigma = m_is2p5 ? (10.0 / (7.0 * M_PI)) : (1.0 / M_PI);
                // Standard cubic spline formula
                if (q < 1.0)
                {
                    return sigma * (1.0 - 1.5 * q * q + 0.75 * q * q * q) / (h * h * h);
                }
                else if (q < 2.0)
                {
                    return sigma * 0.25 * std::pow(2.0 - q, 3) / (h * h * h);
                }
                else
                {
                    return 0.0;
                }
            }

            virtual vec_t dw(const vec_t &rij, const real r, const real h) const override
            {
                const real q = r / h;
                real sigma = m_is2p5 ? (10.0 / (7.0 * M_PI)) : (1.0 / M_PI);
                real dwdr = 0.0;
                if (q < 1.0)
                {
                    dwdr = sigma * (-3.0 * q + 2.25 * q * q) / (h * h * h * h);
                }
                else if (q < 2.0)
                {
                    dwdr = -sigma * 0.75 * std::pow(2.0 - q, 2) / (h * h * h * h);
                }
                return (r > 1e-8 ? rij * (dwdr / r) : vec_t{0.0, 0.0, 0.0});
            }

            virtual real dhw(const real r, const real h) const override
            {
                // Finite difference approximation for dh/dh.
                const real delta = 1e-6 * h;
                real wPlus = w(r, h + delta);
                real wMinus = w(r, h - delta);
                return (wPlus - wMinus) / (2 * delta);
            }

            virtual real W(const real q) const override
            {
                // Dimensionless kernel profile.
                if (q < 1.0)
                    return 1.0 - 1.5 * q * q + 0.75 * q * q * q;
                else if (q < 2.0)
                    return 0.25 * std::pow(2.0 - q, 3);
                else
                    return 0.0;
            }

            virtual real dW_dq(const real q) const override
            {
                if (q < 1.0)
                    return -3.0 * q + 2.25 * q * q;
                else if (q < 2.0)
                    return -0.75 * std::pow(2.0 - q, 2);
                else
                    return 0.0;
            }
        };

    } // namespace Spline
} // namespace sph
