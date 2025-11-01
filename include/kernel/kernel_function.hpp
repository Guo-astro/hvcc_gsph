/* ================================
 * kernel_function.hpp
 * ================================ */
#pragma once

#include "utilities/vector_type.hpp"

namespace sph
{
    inline int kernelDim(bool is2p5)
    {
#if DIM == 1
        return 1;
#elif DIM == 2
        return 2;
#elif DIM == 3
        return is2p5 ? 2 : 3;
#else
#error "Unsupported dimension: must be 1, 2, or 3."
#endif
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
        virtual ~KernelFunction() = default;
    };
}