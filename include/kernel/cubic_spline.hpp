#pragma once

#include <cmath>
#include "defines.hpp"
#include "kernel_function.hpp"

// Cubic spline kernel
namespace sph
{
    namespace Spline
    {
        // Note: We no longer use the compile-time sigma_cubic; we compute it at runtime.
        class Cubic : public KernelFunction
        {
        private:
            // m_is2p5 is true when running in “2.5D” mode (i.e. hydrodynamics treated in 2D)
            bool m_is2p5;

        public:
            // Constructor; pass true if you wish the hydrodynamic kernel to use 2D normalization.
            Cubic(bool is2p5 = false) : m_is2p5(is2p5)
            {
            }

            // Kernel function w(r,h)
            real w(const real r, const real h) const override
            {
                int kd = kernelDim(m_is2p5);
                real sigma = 0.0;
                if (kd == 1)
                    sigma = 2.0 / 3.0;
                else if (kd == 2)
                    sigma = 10.0 / (7.0 * M_PI);
                else
                    sigma = 1.0 / M_PI;
                // Use half of h in the formulation (as in many SPH kernels)
                const real h_ = h * 0.5;
                const real q = r / h_;
                // Denom is h_^kd
                real denom = powh_dim(h_, kd);
                // The kernel is computed using a cubic polynomial.
                // (This expression is based on your original code.)
                return sigma / denom * (0.25 * pow3(0.5 * (2.0 - q + std::abs(2.0 - q))) - pow3(0.5 * (1.0 - q + std::abs(1.0 - q))));
            }

            // Kernel gradient: returns grad W(r,h) as a vector.
            vec_t dw(const vec_t &rij, const real r, const real h) const override
            {
                int kd = kernelDim(m_is2p5);
                real sigma = 0.0;
                if (kd == 1)
                    sigma = 2.0 / 3.0;
                else if (kd == 2)
                    sigma = 10.0 / (7.0 * M_PI);
                else
                    sigma = 1.0 / M_PI;

                if (r == 0.0)
                {
                    return vec_t(0);
                }
                const real h_ = h * 0.5;
                const real q = r / h_;
                // Use our runtime power function for h_ (with effective dimension)
                const real denom = powh_dim(h_, kd);
                const real c = -sigma / (denom * h_ * r) *
                               (0.75 * sqr(0.5 * (2.0 - q + std::abs(2.0 - q))) - 3.0 * sqr(0.5 * (1.0 - q + std::abs(1.0 - q))));
                return rij * c;
            }

            // Derivative of the kernel with respect to h.
            real dhw(const real r, const real h) const override
            {
                int kd = kernelDim(m_is2p5);
                real sigma = 0.0;
                if (kd == 1)
                    sigma = 2.0 / 3.0;
                else if (kd == 2)
                    sigma = 10.0 / (7.0 * M_PI);
                else
                    sigma = 1.0 / M_PI;
                const real h_ = h * 0.5;
                const real q = r / h_;
                // Replace DIM with kd so that the derivative scales with the effective kernel dimension.
                return 0.5 * sigma / (powh_dim(h_, kd) * h_) *
                       (sqr((std::abs(2.0 - q) + 2.0 - q) * 0.5) * ((3.0 + kd) * 0.25 * q - 0.5 * kd) + sqr((std::abs(1.0 - q) + 1.0 - q) * 0.5) * ((-3.0 - kd) * q + kd));
            }
        };
    } // namespace Spline
} // namespace sph
