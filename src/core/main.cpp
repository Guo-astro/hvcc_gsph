#include <iostream>
#include <string>

#include "core/solver.hpp"
#include "utilities/exception.hpp"
#include "core/sample_registry.hpp"

int main(int argc, char *argv[])
{
    std::ios_base::sync_with_stdio(false);
    
    // Handle --list-samples flag
    if (argc >= 2) {
        std::string arg1 = argv[1];
        if (arg1 == "--list-samples" || arg1 == "-l") {
            std::cout << "\nAvailable SPH Simulations:\n";
            std::cout << std::string(80, '=') << "\n\n";
            
            auto& registry = sph::SampleRegistry::instance();
            auto samples = registry.get_all_samples();
            
            if (samples.empty()) {
                std::cout << "No samples registered.\n";
            } else {
                std::cout << "Total samples: " << samples.size() << "\n\n";
                
                // Group samples by category (inferred from name)
                std::cout << "Registered samples:\n";
                for (const auto& name : samples) {
                    std::cout << "  - " << name << "\n";
                }
            }
            
            std::cout << "\n" << std::string(80, '=') << "\n";
            std::cout << "\nUsage: " << argv[0] << " <sampleName> [jsonFile] [numThreads]\n";
            std::cout << "   or: " << argv[0] << " --list-samples\n\n";
            return 0;
        }
        
        if (arg1 == "--help" || arg1 == "-h") {
            std::cout << "\nGSPH Code - Smoothed Particle Hydrodynamics Simulator\n";
            std::cout << std::string(80, '=') << "\n\n";
            std::cout << "Usage: " << argv[0] << " <sampleName> [jsonFile] [numThreads]\n\n";
            std::cout << "Arguments:\n";
            std::cout << "  sampleName   Name of the simulation to run\n";
            std::cout << "  jsonFile     Optional: JSON configuration file\n";
            std::cout << "  numThreads   Optional: Number of OpenMP threads\n\n";
            std::cout << "Options:\n";
            std::cout << "  --list-samples, -l   List all available simulations\n";
            std::cout << "  --help, -h           Show this help message\n\n";
            std::cout << "Examples:\n";
            std::cout << "  " << argv[0] << " shock_tube                    # Run with defaults\n";
            std::cout << "  " << argv[0] << " shock_tube config.json        # Use config file\n";
            std::cout << "  " << argv[0] << " shock_tube config.json 8      # Use 8 threads\n";
            std::cout << "  " << argv[0] << " --list-samples                # Show available simulations\n\n";
            return 0;
        }
    }
    
    sph::exception_handler([&]()
                           {
        sph::Solver solver(argc, argv);
        solver.run(); });
    return 0;
}