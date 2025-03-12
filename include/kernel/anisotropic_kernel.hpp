#pragma once

#include <cmath>
#include <stdexcept>
#include "kernel_function.hpp"
#include "vector_type.hpp"
#include "defines.hpp"

namespace sph
{
    namespace Anisotropic
    {

        // 2D Wendland C4 Kernel for the disk-plane.
        inline real W2D(const real q)
        {
            if (q >= 1.0)
                return 0.0;
            return pow6(0.5 * (1.0 - q + std::abs(1.0 - q))) *
                   (1.0 + 6.0 * q + (35.0 / 3.0) * q * q);
        }

        inline real dW2D_dq(const real q)
        {
            if (q >= 1.0)
                return 0.0;
            real term = 0.5 * (1.0 - q + std::abs(1.0 - q)); // = (1 - q) for q < 1
            real dterm_dq = -0.5;
            return 6.0 * pow5(term) * dterm_dq * (1.0 + 6.0 * q + (35.0 / 3.0) * q * q) +
                   pow6(term) * (6.0 + (70.0 / 3.0) * q);
        }

        // 1D Gaussian Kernel for the vertical (z) direction.
        inline real W1D(const real z, const real h_z)
        {
            return 1.0 / (std::sqrt(2.0 * M_PI) * h_z) * std::exp(-0.5 * (z * z) / (h_z * h_z));
        }

        inline real dW1D_dz(const real z, const real h_z)
        {
            real W1 = W1D(z, h_z);
            return -(z / (h_z * h_z)) * W1;
        }

        // The product anisotropic kernel: W(x,y,z; h_xy, h_z) = W2D(r_xy; h_xy) * W1D(z; h_z)
        class AnisotropicKernel : public KernelFunction
        {
        public:
            explicit AnisotropicKernel(real hz) : m_hz(hz)
            {
                if (hz <= 0)
                    throw std::invalid_argument("h_z must be positive");
            }

            // Compute the anisotropic kernel value.
            real wAniso(const vec_t &rij, const real h_xy) const
            {
                if (h_xy <= 0)
                    throw std::invalid_argument("h_xy must be positive");
                real r_xy = std::sqrt(rij[0] * rij[0] + rij[1] * rij[1]);
                real q_xy = r_xy / h_xy;
                const real sigma_2d = 9.0 / M_PI; // Normalization for 2D Wendland C4
                return (sigma_2d / (h_xy * h_xy)) * W2D(q_xy) * W1D(rij[2], m_hz);
            }

            // Compute the gradient for the anisotropic kernel.
            vec_t dwAniso(const vec_t &rij, const real h_xy) const
            {
                if (h_xy <= 0)
                    throw std::invalid_argument("h_xy must be positive");
                real x = rij[0], y = rij[1], z = rij[2];
                real r_xy = std::sqrt(x * x + y * y);
                real q_xy = r_xy / h_xy;
                real dW2D_dr = 0.0;
                const real sigma_2d = 9.0 / M_PI;
                if (q_xy < 1.0)
                {
                    dW2D_dr = sigma_2d * dW2D_dq(q_xy) / (h_xy * h_xy * h_xy);
                }
                vec_t grad_xy = {0.0, 0.0, 0.0};
                if (r_xy > 1e-8)
                {
                    grad_xy[0] = dW2D_dr * (x / r_xy);
                    grad_xy[1] = dW2D_dr * (y / r_xy);
                }
                real W1 = W1D(z, m_hz);
                real dW1_dz = dW1D_dz(z, m_hz);
                real W2 = sigma_2d * W2D(q_xy) / (h_xy * h_xy);
                vec_t grad = {grad_xy[0] * W1, grad_xy[1] * W1, W2 * dW1_dz};
                return grad;
            }

            // Finite-difference approximation for derivative with respect to h_xy.
            real dhwAniso(const vec_t &rij, const real h_xy) const
            {
                if (h_xy <= 0)
                    throw std::invalid_argument("h_xy must be positive");
                const real delta = 1e-6 * h_xy;
                real wPlus = wAniso(rij, h_xy + delta);
                real wMinus = wAniso(rij, h_xy - delta);
                return (wPlus - wMinus) / (2.0 * delta);
            }

            // For the standard isotropic interface.
            real w(const real r, const real h) const override
            {
                throw std::runtime_error("Isotropic w(r, h) not applicable for anisotropic kernel; use wAniso");
            }

            vec_t dw(const vec_t &rij, const real r, const real h) const override
            {
                return dwAniso(rij, h); // Delegate to anisotropic gradient, assuming h = h_xy
            }

            real dhw(const real r, const real h) const override
            {
                throw std::runtime_error("Isotropic dhw(r, h) not applicable for anisotropic kernel; use dhwAniso");
            }

            // Dimensionless kernel profile W(q)
            real W(const real q) const override
            {
                // Delegate to W2D for consistency, as W(q) is used in anisotropic computations
                return W2D(q);
            }

            // Derivative of W w.r.t. q
            real dW_dq(const real q) const override
            {
                return dW2D_dq(q);
            }

        private:
            real m_hz;
        };

    } // namespace Anisotropic
} // namespace sph