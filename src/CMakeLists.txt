target_sources(sph_lib
    PRIVATE
        main.cpp
        bhtree.cpp
        exhaustive_search.cpp
        fluid_force.cpp
        gravity_force.cpp
        logger.cpp
        output.cpp
        pre_interaction.cpp
        sample_registry.cpp
        simulation.cpp
        solver.cpp
        timestep.cpp
        heating_cooling.cpp
        inplane_integration.cpp
        shock_detection.cpp
)

# Add subdirectories containing additional source files.
add_subdirectory(disph)
add_subdirectory(gdisph)
add_subdirectory(gsph)
add_subdirectory(sample)
add_subdirectory(production_sims)
add_subdirectory(relaxation)

# Optionally, copy sample data into the build directory.
file(COPY ${CMAKE_SOURCE_DIR}/sample DESTINATION ${CMAKE_BINARY_DIR})
file(COPY ${CMAKE_SOURCE_DIR}/production_sims DESTINATION ${CMAKE_BINARY_DIR})
