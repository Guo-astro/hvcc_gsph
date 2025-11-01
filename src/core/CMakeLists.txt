# Core simulation framework sources
# NOTE: main.cpp is NOT included here - it's compiled separately for the executable
target_sources(sph_lib
    PRIVATE
        simulation.cpp
        solver.cpp
        output_format.cpp
        simulation_run.cpp
        logger.cpp
        sample_registry.cpp
)
