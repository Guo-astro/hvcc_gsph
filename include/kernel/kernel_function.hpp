/* ================================
 * kernel_function.hpp
 * ================================ */
#pragma once

#include "vector_type.hpp"

namespace sph
{
    inline int kernelDim(bool two_and_half)
    {
        return two_and_half ? 2 : DIM;
    }
    inline real powh_dim(real h, int kd)
    {
        if (kd == 1)
            return h;
        if (kd == 2)
            return h * h;
        return h * h * h; // default 3
    }
    inline real powh_(real h, int dim)
    {
        return std::pow(h, dim - 1);
    }
    class KernelFunction
    {
    public:
        virtual real w(const real r, const real h) const = 0;                     // W(r,h)
        virtual vec_t dw(const vec_t &rij, const real r, const real h) const = 0; // grad W(r,h)
        virtual real dhw(const real r, const real h) const = 0;                   // dW(r,h)/dh
    };

}
