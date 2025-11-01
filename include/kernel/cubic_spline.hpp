#pragma once

#include <cmath>
#include <cassert>

#include "utilities/defines.hpp"
#include "kernel_function.hpp"
#include "utilities/vector_type.hpp"

namespace sph
{
    namespace Spline
    {

        ////////////////////////////////////////////////////////////////////////////////
        // 1) Helpers to handle dimension & normalization
        ////////////////////////////////////////////////////////////////////////////////

        // Returns the effective kernel dimension. If DIM==3 and is2p5==true, we use kd=2.
        inline int kernelDim(bool is2p5)
        {
#if DIM == 1
            return 1;
#elif DIM == 2
            return 2;
#elif DIM == 3
            return (is2p5 ? 2 : 3);
#else
#error "Unsupported dimension: must be 1, 2, or 3."
#endif
        }

        // Dimension-specific normalization constants for the cubic spline.
        inline real sigmaCubic(int kd)
        {
            switch (kd)
            {
            case 1:
                return 2.0 / 3.0; // 1D
            case 2:
                return 10.0 / (7.0 * M_PI); // 2D
            default:
                return 1.0 / M_PI; // 3D
            }
        }

        // Raises h to the power kd. (kd=1 => h^1, kd=2 => h^2, kd=3 => h^3)
        inline real powhDim(real h, int kd)
        {
            switch (kd)
            {
            case 1:
                return h; // h^1
            case 2:
                return h * h; // h^2
            default:
                return h * h * h; // h^3
            }
        }

        ////////////////////////////////////////////////////////////////////////////////
        // 2) The Cubic spline kernel, preserving your "original" logic
        ////////////////////////////////////////////////////////////////////////////////
        class Cubic : public KernelFunction
        {
        private:
            // If true (and DIM==3), pretend the kernel dimension is 2 => "2.5D".
            bool m_is2p5;

        public:
            // Optionally enable 2.5D mode if you're really in 3D but want 2D normalization.
            explicit Cubic(bool is2p5 = false) : m_is2p5(is2p5) {}

            ////////////////////////////////////////////////////////////////////////////
            // W(r, h): the *scaled* kernel value
            ////////////////////////////////////////////////////////////////////////////
            virtual real w(const real r, const real h) const override
            {
                // 1) Determine effective dimension
                const int kd = kernelDim(m_is2p5);
                // 2) Normalization
                const real sigma = sigmaCubic(kd);

                // 3) Half-smoothing length
                const real h_ = 0.5 * h;
                // 4) Dimensionless distance
                const real q = r / h_;

                // 5) Evaluate the difference of two "max(0, ...)" cubic terms
                //    as in your old code:
                //    0.25 * [ (2-q)^3 ] - [ (1-q)^3 ], using abs(...) to do piecewise
                return sigma / powhDim(h_, kd) * (0.25 * pow3(0.5 * (2.0 - q + std::abs(2.0 - q))) - pow3(0.5 * (1.0 - q + std::abs(1.0 - q))));
            }

            ////////////////////////////////////////////////////////////////////////////
            // grad W(r, h): the gradient w.r.t. r
            ////////////////////////////////////////////////////////////////////////////
            virtual vec_t dw(const vec_t &rij, const real r, const real h) const override
            {
                // Avoid division by zero
                if (r == 0.0)
                {
#if DIM == 3
                    return vec_t{0.0, 0.0, 0.0};
#elif DIM == 2
                    return vec_t{0.0, 0.0};
#elif DIM == 1
                    return vec_t{0.0};
#endif
                }

                // 1) Dimension & normalization
                const int kd = kernelDim(m_is2p5);
                const real sigma = sigmaCubic(kd);

                // 2) Same half-smoothing length
                const real h_ = 0.5 * h;
                const real q = r / h_;

                // 3) Old code formula, but ensuring we do h_^kd (not h_^2 for 3D).
                const real c = -sigma / (powhDim(h_, kd) * h_ * r) * (0.75 * sqr(0.5 * (2.0 - q + std::abs(2.0 - q))) - 3.0 * sqr(0.5 * (1.0 - q + std::abs(1.0 - q))));

                // Multiply by the unit vector in direction of r
                return rij * c;
            }

            ////////////////////////////////////////////////////////////////////////////
            // dW(r, h)/dh: derivative w.r.t. smoothing length
            ////////////////////////////////////////////////////////////////////////////
            virtual real dhw(const real r, const real h) const override
            {
                // 1) dimension & normalization
                const int kd = kernelDim(m_is2p5);
                const real sigma = sigmaCubic(kd);

                // 2) half-h
                const real h_ = 0.5 * h;
                const real q = r / h_;

                // 3) Same expression from your old code, but with powhDim(h_, kd).
                return 0.5 * sigma / (powhDim(h_, kd) * h_) * (sqr((std::abs(2.0 - q) + 2.0 - q) * 0.5) * ((3.0 + kd) * 0.25 * q - 0.5 * kd) + sqr((std::abs(1.0 - q) + 1.0 - q) * 0.5) * ((-3.0 - kd) * q + kd));
            }
        };

    } // namespace Spline
} // namespace sph
