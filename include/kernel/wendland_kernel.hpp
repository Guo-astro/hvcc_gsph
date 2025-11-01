#pragma once

#include <cmath>
#include <cassert>

#include "utilities/defines.hpp"
#include "kernel_function.hpp"

namespace sph
{
    namespace Wendland
    {

        // Wendland C4 Kernel (Dehnen & Aly 2012)
        // This kernel is defined for DIM==1, 2, or 3.
        // In 2.5D mode (m_is2p5==true), we use an effective kernel dimension of 2.
        class C4Kernel : public KernelFunction
        {
        private:
            // When true, hydrodynamics are computed as if in 2D.
            bool m_is2p5;

        public:
            // Constructor accepts a flag; if true, the effective kernel dimension is 2.
            explicit C4Kernel(bool is2p5 = false) : m_is2p5(is2p5)
            {
                // Previously, Wendland C4 was not defined for DIM==1.
                // For our new code we now support DIM==1 with an appropriate normalization.
                // (No assertion is made here.)
            }

            // Kernel value w(r, h)
            virtual real w(const real r, const real h) const override
            {
                int kd = kernelDim(m_is2p5); // effective kernel dimension: if DIM==1 then kd==1, etc.
                real sigma = 0.0;
                if (kd == 1)
                    sigma = 27.0 / 16.0; // Normalization for 1D (computed to yield unit integral)
                else if (kd == 2)
                    sigma = 9.0 / M_PI;
                else // kd == 3
                    sigma = 495.0 / (32.0 * M_PI);

                // q = r/h, and note that 0.5*(1 - q + |1 - q|) equals max(0, 1 - q)
                const real q = r / h;
                if (q >= 1.0)
                    return 0.0; // Compact support

                return sigma / powh_dim(h, kd) *
                       pow6(0.5 * (1.0 - q + std::abs(1.0 - q))) *
                       (1.0 + 6.0 * q + (35.0 / 3.0) * q * q);
            }

            // Kernel gradient: returns the gradient of w with respect to r.
            virtual vec_t dw(const vec_t &rij, const real r, const real h) const override
            {
                int kd = kernelDim(m_is2p5);
                real sigma = 0.0;
                if (kd == 1)
                    sigma = 27.0 / 16.0;
                else if (kd == 2)
                    sigma = 9.0 / M_PI;
                else
                    sigma = 495.0 / (32.0 * M_PI);

                const real q = r / h;
                if (q >= 1.0 || r <= 1e-8)
                {
#if DIM == 3
                    return vec_t{0.0, 0.0, 0.0}; // For 3D
#elif DIM == 2
                    return vec_t{0.0, 0.0}; // For 2D
#elif DIM == 1
                    return vec_t{0.0}; // For 1D
#endif
                }
                // Use the effective power for h: powh_dim(h, kd)
                const real denom = powh_dim(h, kd);
                const real c = -56.0 / 3.0 * sigma / (denom * h * h) *
                               pow5(0.5 * (1.0 - q + std::abs(1.0 - q))) *
                               (1.0 + 5.0 * q);
                // Previously, an extra division by r was applied here.
                // Since c is defined such that 'rij * c' yields the correct gradient,
                // we now simply return:
                return rij * c;
            }

            // Derivative of the kernel with respect to h.
            virtual real dhw(const real r, const real h) const override
            {
                int kd = kernelDim(m_is2p5);
                real sigma = 0.0;
                if (kd == 1)
                    sigma = 27.0 / 16.0;
                else if (kd == 2)
                    sigma = 9.0 / M_PI;
                else
                    sigma = 495.0 / (32.0 * M_PI);

                const real q = r / h;
                if (q >= 1.0)
                    return 0.0; // Compact support

                return -sigma / (powh_dim(h, kd) * h * 3.0) *
                       pow5(0.5 * (1.0 - q + std::abs(1.0 - q))) *
                       (3.0 * kd + 15.0 * kd * q + (-56.0 + 17.0 * kd) * q * q - 35.0 * (8.0 + kd) * q * q * q);
            }
        };

    } // namespace Wendland
} // namespace sph
