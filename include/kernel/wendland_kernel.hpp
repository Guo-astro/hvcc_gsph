#pragma once

#include <cmath>
#include <cassert>

#include "defines.hpp"
#include "kernel_function.hpp"

namespace sph
{
    namespace Wendland
    {

        // Wendland C4 Kernel (Dehnen & Aly 2012)
        // This kernel is normally defined for DIM==2 or 3.
        // In 2.5D mode (m_is2p5==true), we use an effective kernel dimension of 2.
        class C4Kernel : public KernelFunction
        {
        private:
            // When true, hydrodynamics are computed as if in 2D.
            bool m_is2p5;

        public:
            // Constructor accepts a flag; if true, the effective kernel dimension is 2.
            C4Kernel(bool is2p5 = false) : m_is2p5(is2p5)
            {
                // Wendland C4 is not defined for DIM==1.
                assert(DIM != 1);
            }

            // Kernel value w(r, h)
            real w(const real r, const real h) const override
            {
                int kd = kernelDim(m_is2p5);
                real sigma = 0.0;
                if (kd == 1)
                    sigma = 0.0; // Not used.
                else if (kd == 2)
                    sigma = 9.0 / M_PI;
                else // kd == 3
                    sigma = 495.0 / (32.0 * M_PI);
                // Here q = r/h (the Wendland kernel is typically written in terms of q=r/h)
                const real q = r / h;
                if (q >= 1.0)
                    return 0.0; // Compact support
                // Use the effective power for h: powh_dim(h, kd)
                return sigma / powh_dim(h, kd) *
                       pow6(0.5 * (1.0 - q + std::abs(1.0 - q))) *
                       (1.0 + 6.0 * q + (35.0 / 3.0) * q * q);
            }

            // Kernel gradient: returns the gradient of w with respect to r.
            vec_t dw(const vec_t &rij, const real r, const real h) const override
            {
                int kd = kernelDim(m_is2p5);
                real sigma = 0.0;
                if (kd == 1)
                    sigma = 0.0;
                else if (kd == 2)
                    sigma = 9.0 / M_PI;
                else
                    sigma = 495.0 / (32.0 * M_PI);
                const real q = r / h;
                if (q >= 1.0 || r <= 1e-8)
                    return vec_t{0.0, 0.0, 0.0}; // Compact support and avoid division by zero
                // Here we use powh_dim(h, kd) for the normalization.
                const real denom = powh_dim(h, kd);
                const real c = -56.0 / 3.0 * sigma / (denom * h * h) *
                               pow5(0.5 * (1.0 - q + std::abs(1.0 - q))) *
                               (1.0 + 5.0 * q);
                return rij * (c / r); // Scale by direction
            }

            // Derivative of the kernel with respect to h.
            real dhw(const real r, const real h) const override
            {
                int kd = kernelDim(m_is2p5);
                real sigma = 0.0;
                if (kd == 1)
                    sigma = 0.0;
                else if (kd == 2)
                    sigma = 9.0 / M_PI;
                else
                    sigma = 495.0 / (32.0 * M_PI);
                const real q = r / h;
                if (q >= 1.0)
                    return 0.0; // Compact support
                // Replace the compile-time DIM with our effective dimension kd.
                return -sigma / (powh_dim(h, kd) * h * 3.0) *
                       pow5(0.5 * (1.0 - q + std::abs(1.0 - q))) *
                       (3.0 * kd + 15.0 * kd * q + (-56.0 + 17.0 * kd) * q * q - 35.0 * (8.0 + kd) * q * q * q);
            }

            // Dimensionless kernel profile W(q)
            real W(const real q) const override
            {
                if (q >= 1.0)
                    return 0.0; // Compact support
                return pow6(0.5 * (1.0 - q + std::abs(1.0 - q))) *
                       (1.0 + 6.0 * q + (35.0 / 3.0) * q * q);
            }

            // Derivative of W w.r.t. q
            real dW_dq(const real q) const override
            {
                if (q >= 1.0)
                    return 0.0;                                  // Compact support
                real term = 0.5 * (1.0 - q + std::abs(1.0 - q)); // = (1 - q) for q < 1
                real dterm_dq = -0.5;                            // For q < 1
                // d/dq [term^6 * (1 + 6q + (35/3)q^2)] = 6 * term^5 * dterm_dq * (1 + 6q + (35/3)q^2) + term^6 * (6 + (70/3)q)
                return 6.0 * pow5(term) * dterm_dq * (1.0 + 6.0 * q + (35.0 / 3.0) * q * q) +
                       pow6(term) * (6.0 + (70.0 / 3.0) * q);
            }
        };

    } // namespace Wendland
} // namespace sph