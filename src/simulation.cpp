/* ================================
 * simulation.cpp
 * ================================ */
#include "parameters.hpp"
#include "simulation.hpp"
#include "exception.hpp"
#include "periodic.hpp"
#include "bhtree.hpp"
#include "kernel/cubic_spline.hpp"
#include "kernel/wendland_kernel.hpp"

namespace sph
{

    Simulation::Simulation(std::shared_ptr<SPHParameters> param)
    {
        m_periodic = std::make_shared<Periodic>();
        m_periodic->initialize(param);
        // pick kernel
        bool is2p5 = param->two_and_half_sim;
        if (param->kernel == KernelType::CUBIC_SPLINE)
        {
            m_kernel = std::make_shared<Spline::Cubic>(is2p5);
        }
        else if (param->kernel == KernelType::WENDLAND)
        {
            m_kernel = std::make_shared<Wendland::C4Kernel>(is2p5);
        }
        else
        {
            THROW_ERROR("Unknown kernel.");
        }

        m_tree = std::make_shared<BHTree>();
        m_tree->initialize(param);

        m_time = param->time.start;
        m_dt = 0.0;
    }

    void Simulation::update_time()
    {
        m_time += m_dt;
    }

    void Simulation::make_tree()
    {
        m_tree->make(m_particles, m_particle_num);
    }

    void Simulation::add_scalar_array(const std::vector<std::string> &names)
    {
        const int num = m_particle_num;
        for (const auto &name : names)
        {
            additional_scalar_array[name].resize(num);
        }
    }

    void Simulation::add_vector_array(const std::vector<std::string> &names)
    {
        const int num = m_particle_num;
        for (const auto &name : names)
        {
            additional_vector_array[name].resize(num);
        }
    }

    std::vector<real> &Simulation::get_scalar_array(const std::string &name)
    {
        auto it = additional_scalar_array.find(name);
        if (it != additional_scalar_array.end())
        {
            return it->second;
        }
        else
        {
            THROW_ERROR("additional_scalar_array does not have ", name);
        }
    }

    std::vector<vec_t> &Simulation::get_vector_array(const std::string &name)
    {
        auto it = additional_vector_array.find(name);
        if (it != additional_vector_array.end())
        {
            return it->second;
        }
        else
        {
            THROW_ERROR("additional_vector_array does not have ", name);
        }
    }

}